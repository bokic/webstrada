/**
 * @file llvm_compiler.cpp
 * @brief LLVM JIT compiler driver.
 *
 * The llvm_codegen class: the constructor registers every runtime helper
 * symbol with LLVM's DynamicLibrary (the AddSymbol table) and compile()
 * drives a template through parsing -> IR generation -> MCJIT. The actual
 * tag/expression code generation lives in src/codegen/llvm_codegen.cpp and
 * the src/codegen/codegen_*.cpp files.
 */

#include "codegen_internal.h"

#include <webstrada/llvm_codegen.h>
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/parser.h>
#include <webstrada/template_reader.h>
#include <webstrada/worker.h>
#include <webstrada/cf8.h>
#include <webstrada/cfimage.h>

#include <unordered_set>
#include <llvm/ADT/StringRef.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>

#include <dlfcn.h>

#include <textparser.hpp>
#include <cfml_definition.json.h>

using namespace webstrada;

namespace webstrada {

namespace {
// Scopes g_textparser to the compile of one template/component: codegen helpers
// (lineOfOffset, via textparser_get_line_number_at_position) resolve the active
// textparser's line map while this is live.
struct TextparserHandleGuard {
    textparser_t saved;
    explicit TextparserHandleGuard(textparser_t h) : saved(g_textparser) { g_textparser = h; }
    ~TextparserHandleGuard() { g_textparser = saved; }
};
}

llvm_codegen::llvm_codegen()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    llvm::sys::DynamicLibrary::AddSymbol("cfwriteoutput", reinterpret_cast<void*>(cfml::cfwriteoutput));
    llvm::sys::DynamicLibrary::AddSymbol("cf_whitespace_space", reinterpret_cast<void*>(cfml::cf_whitespace_space));
    llvm::sys::DynamicLibrary::AddSymbol("cf_silent_begin", reinterpret_cast<void*>(cfml::cf_silent_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_silent_end", reinterpret_cast<void*>(cfml::cf_silent_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_write_output_gated", reinterpret_cast<void*>(cfml::cf_write_output_gated));
    llvm::sys::DynamicLibrary::AddSymbol("cf_whitespace_space_gated", reinterpret_cast<void*>(cfml::cf_whitespace_space_gated));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setting", reinterpret_cast<void*>(cfml::cf_setting));
    llvm::sys::DynamicLibrary::AddSymbol("cf_htmlhead_append", reinterpret_cast<void*>(cfml::cf_htmlhead_append));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cookie_tag", reinterpret_cast<void*>(cfml::cf_cookie_tag));
    llvm::sys::DynamicLibrary::AddSymbol("cf_savecontent_begin", reinterpret_cast<void*>(cfml::cf_savecontent_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_savecontent_validate", reinterpret_cast<void*>(cfml::cf_savecontent_validate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_savecontent_end_assign", reinterpret_cast<void*>(cfml::cf_savecontent_end_assign));
    llvm::sys::DynamicLibrary::AddSymbol("cf_savecontent_end", reinterpret_cast<void*>(cfml::cf_savecontent_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_login_begin", reinterpret_cast<void*>(cfml::cf_login_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_login_end", reinterpret_cast<void*>(cfml::cf_login_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_loginuser", reinterpret_cast<void*>(cfml::cf_loginuser));
    llvm::sys::DynamicLibrary::AddSymbol("cf_logout", reinterpret_cast<void*>(cfml::cf_logout));
    llvm::sys::DynamicLibrary::AddSymbol("cf_set_login_storage", reinterpret_cast<void*>(cfml::cf_set_login_storage));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xml_begin", reinterpret_cast<void*>(cfml::cf_xml_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xml_end", reinterpret_cast<void*>(cfml::cf_xml_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_query_begin", reinterpret_cast<void*>(cfml::cf_query_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_http_begin", reinterpret_cast<void*>(cfml::cf_http_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_http_param", reinterpret_cast<void*>(cfml::cf_http_param));
    llvm::sys::DynamicLibrary::AddSymbol("cf_http_end", reinterpret_cast<void*>(cfml::cf_http_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_transaction_begin", reinterpret_cast<void*>(cfml::cf_transaction_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_transaction_commit", reinterpret_cast<void*>(cfml::cf_transaction_commit));
    llvm::sys::DynamicLibrary::AddSymbol("cf_transaction_rollback", reinterpret_cast<void*>(cfml::cf_transaction_rollback));    llvm::sys::DynamicLibrary::AddSymbol("cf_query_end", reinterpret_cast<void*>(cfml::cf_query_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_query_rowcount", reinterpret_cast<void*>(cfml::cf_query_rowcount));
    llvm::sys::DynamicLibrary::AddSymbol("cf_query_set_row", reinterpret_cast<void*>(cfml::cf_query_set_row));
    llvm::sys::DynamicLibrary::AddSymbol("cf_query_resolve", reinterpret_cast<void*>(cfml::cf_query_resolve));
    llvm::sys::DynamicLibrary::AddSymbol("cf_query_scope_push", reinterpret_cast<void*>(cfml::cf_query_scope_push));
    llvm::sys::DynamicLibrary::AddSymbol("cf_query_scope_pop", reinterpret_cast<void*>(cfml::cf_query_scope_pop));
    llvm::sys::DynamicLibrary::AddSymbol("cf_query_group_next", reinterpret_cast<void*>(cfml::cf_query_group_next));
    llvm::sys::DynamicLibrary::AddSymbol("cf_queryparam", reinterpret_cast<void*>(cfml::cf_queryparam));
    llvm::sys::DynamicLibrary::AddSymbol("cf_insert_tag", reinterpret_cast<void*>(cfml::cf_insert_tag));
    llvm::sys::DynamicLibrary::AddSymbol("cf_update", reinterpret_cast<void*>(cfml::cf_update));
    llvm::sys::DynamicLibrary::AddSymbol("cf_dbinfo", reinterpret_cast<void*>(cfml::cf_dbinfo));
    llvm::sys::DynamicLibrary::AddSymbol("cf_storedproc_begin", reinterpret_cast<void*>(cfml::cf_storedproc_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_proc_param", reinterpret_cast<void*>(cfml::cf_proc_param));
    llvm::sys::DynamicLibrary::AddSymbol("cf_proc_result", reinterpret_cast<void*>(cfml::cf_proc_result));
    llvm::sys::DynamicLibrary::AddSymbol("cf_storedproc_end", reinterpret_cast<void*>(cfml::cf_storedproc_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_directory_tag", reinterpret_cast<void*>(cfml::cf_directory_tag));
    llvm::sys::DynamicLibrary::AddSymbol("cf_file_tag", reinterpret_cast<void*>(cfml::cf_file_tag));
    llvm::sys::DynamicLibrary::AddSymbol("cf_execute_tag", reinterpret_cast<void*>(cfml::cf_execute_tag));
    llvm::sys::DynamicLibrary::AddSymbol("cf_wddx_tag", reinterpret_cast<void*>(cfml::cf_wddx_tag));
    llvm::sys::DynamicLibrary::AddSymbol("cf_feed_tag", reinterpret_cast<void*>(cfml::cf_feed_tag));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ftp_tag", reinterpret_cast<void*>(cfml::cf_ftp_tag));
    llvm::sys::DynamicLibrary::AddSymbol("cf_schedule_tag", reinterpret_cast<void*>(cfml::cf_schedule_tag));
    llvm::sys::DynamicLibrary::AddSymbol("cf_zip_begin", reinterpret_cast<void*>(cfml::cf_zip_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_zip_param", reinterpret_cast<void*>(cfml::cf_zip_param));
    llvm::sys::DynamicLibrary::AddSymbol("cf_zip_end", reinterpret_cast<void*>(cfml::cf_zip_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_response_flush", reinterpret_cast<void*>(cfml::response_flush));
    llvm::sys::DynamicLibrary::AddSymbol("cf_response_apply_cfcontent", reinterpret_cast<void*>(cfml::response_apply_cfcontent));
    llvm::sys::DynamicLibrary::AddSymbol("cf_response_add_header", reinterpret_cast<void*>(cfml::response_add_header));
    llvm::sys::DynamicLibrary::AddSymbol("cf_response_redirect", reinterpret_cast<void*>(cfml::response_redirect));
    llvm::sys::DynamicLibrary::AddSymbol("cfdump", reinterpret_cast<void*>(cfml::cfdump));
    llvm::sys::DynamicLibrary::AddSymbol("cfset", reinterpret_cast<void*>(cfml::cfset));
    llvm::sys::DynamicLibrary::AddSymbol("cfoutputexpr", reinterpret_cast<void*>(cfml::cfoutputexpr));
    llvm::sys::DynamicLibrary::AddSymbol("cfgetvar", reinterpret_cast<void*>(cfml::cfgetvar));
    llvm::sys::DynamicLibrary::AddSymbol("cfevalbool", reinterpret_cast<void*>(cfml::cfevalbool));
    llvm::sys::DynamicLibrary::AddSymbol("cfloop_resolve_int", reinterpret_cast<void*>(cfml::cfloop_resolve_int));
    llvm::sys::DynamicLibrary::AddSymbol("cfloop_set_int", reinterpret_cast<void*>(cfml::cfloop_set_int));
    llvm::sys::DynamicLibrary::AddSymbol("cfloop_set_long", reinterpret_cast<void*>(cfml::cfloop_set_long));
    llvm::sys::DynamicLibrary::AddSymbol("cfloop_assign_index", reinterpret_cast<void*>(cfml::cfloop_assign_index));
    llvm::sys::DynamicLibrary::AddSymbol("cfabort", reinterpret_cast<void*>(cfml::cfabort));
    llvm::sys::DynamicLibrary::AddSymbol("cf_exit", reinterpret_cast<void*>(cfml::cf_exit));
    llvm::sys::DynamicLibrary::AddSymbol("cf_exit_loop", reinterpret_cast<void*>(cfml::cf_exit_loop));
    llvm::sys::DynamicLibrary::AddSymbol("cf_exit_invalid", reinterpret_cast<void*>(cfml::cf_exit_invalid));
    llvm::sys::DynamicLibrary::AddSymbol("cf_exit_classify", reinterpret_cast<void*>(cfml::cf_exit_classify));
    llvm::sys::DynamicLibrary::AddSymbol("cf_custom_tag_begin", reinterpret_cast<void*>(cfml::cf_custom_tag_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_custom_tag_end_mode", reinterpret_cast<void*>(cfml::cf_custom_tag_end_mode));
    llvm::sys::DynamicLibrary::AddSymbol("cf_custom_tag_finish", reinterpret_cast<void*>(cfml::cf_custom_tag_finish));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cfoutput_begin", reinterpret_cast<void*>(cfml::cf_cfoutput_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cfoutput_end", reinterpret_cast<void*>(cfml::cf_cfoutput_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_custom_tag_should_loop", reinterpret_cast<void*>(cfml::cf_custom_tag_should_loop));
    llvm::sys::DynamicLibrary::AddSymbol("cf_custom_tag_should_skip_body", reinterpret_cast<void*>(cfml::cf_custom_tag_should_skip_body));
    llvm::sys::DynamicLibrary::AddSymbol("cf_custom_tag_mark_content_changed", reinterpret_cast<void*>(cfml::cf_custom_tag_mark_content_changed));
    llvm::sys::DynamicLibrary::AddSymbol("cf_custom_tag_module_path", reinterpret_cast<void*>(cfml::cf_custom_tag_module_path));
    llvm::sys::DynamicLibrary::AddSymbol("cf_custom_tag_merge_attributecollection", reinterpret_cast<void*>(cfml::cf_custom_tag_merge_attributecollection));
    llvm::sys::DynamicLibrary::AddSymbol("cf_custom_tag_invoke", reinterpret_cast<void*>(cfml::cf_custom_tag_invoke));

    // Itanium C++ exception personality used by all JIT functions' landing pads.
    // Registered via DynamicLibrary so MCJIT can resolve it at JIT time. Resolved
    // with dlsym (not a direct &__gxx_personality_v0 reference) to avoid a
    // PC-relative relocation against the personality in a read-only section.
    llvm::sys::DynamicLibrary::AddSymbol("__gxx_personality_v0", dlsym(RTLD_DEFAULT, "__gxx_personality_v0"));
    llvm::sys::DynamicLibrary::AddSymbol("cf_eh_capture", reinterpret_cast<void*>(cfml::cf_eh_capture));
    llvm::sys::DynamicLibrary::AddSymbol("cf_eh_matches", reinterpret_cast<void*>(cfml::cf_eh_matches));
    llvm::sys::DynamicLibrary::AddSymbol("cf_eh_best_match", reinterpret_cast<void*>(cfml::cf_eh_best_match));
    llvm::sys::DynamicLibrary::AddSymbol("cf_eh_throw", reinterpret_cast<void*>(cfml::cf_eh_throw));
    llvm::sys::DynamicLibrary::AddSymbol("cf_eh_throw_new", reinterpret_cast<void*>(cfml::cf_eh_throw_new));
    llvm::sys::DynamicLibrary::AddSymbol("cf_stack_push", reinterpret_cast<void*>(cfml::cf_stack_push));
    llvm::sys::DynamicLibrary::AddSymbol("cf_stack_set_line", reinterpret_cast<void*>(cfml::cf_stack_set_line));
    llvm::sys::DynamicLibrary::AddSymbol("cf_stack_pop", reinterpret_cast<void*>(cfml::cf_stack_pop));
    llvm::sys::DynamicLibrary::AddSymbol("cf_stack_capture_on_exception", reinterpret_cast<void*>(cfml::cf_stack_capture_on_exception));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_cleanup_save", reinterpret_cast<void*>(cfml::cfvariant_cleanup_save));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_cleanup_restore", reinterpret_cast<void*>(cfml::cfvariant_cleanup_restore));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_cleanup_restore_except", reinterpret_cast<void*>(cfml::cfvariant_cleanup_restore_except));
    llvm::sys::DynamicLibrary::AddSymbol("cf_register_temp", reinterpret_cast<void*>(cfml::cf_register_temp));

    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_null", reinterpret_cast<void*>(cfml::cfvariant_create_null));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_int", reinterpret_cast<void*>(cfml::cfvariant_create_int));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_long", reinterpret_cast<void*>(cfml::cfvariant_create_long));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_float", reinterpret_cast<void*>(cfml::cfvariant_create_float));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_float_literal", reinterpret_cast<void*>(cfml::cfvariant_create_float_literal));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_bool", reinterpret_cast<void*>(cfml::cfvariant_create_bool));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_bool_literal", reinterpret_cast<void*>(cfml::cfvariant_create_bool_literal));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_string", reinterpret_cast<void*>(cfml::cfvariant_create_string));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_array", reinterpret_cast<void*>(cfml::cfvariant_create_array));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_struct", reinterpret_cast<void*>(cfml::cfvariant_create_struct));
    llvm::sys::DynamicLibrary::AddSymbol("cf_abs", reinterpret_cast<void*>(cfml::cf_abs));
    llvm::sys::DynamicLibrary::AddSymbol("cf_asc", reinterpret_cast<void*>(cfml::cf_asc));
    llvm::sys::DynamicLibrary::AddSymbol("cf_chr", reinterpret_cast<void*>(cfml::cf_chr));
    llvm::sys::DynamicLibrary::AddSymbol("cf_acos", reinterpret_cast<void*>(cfml::cf_acos));
    llvm::sys::DynamicLibrary::AddSymbol("cf_asin", reinterpret_cast<void*>(cfml::cf_asin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_atan", reinterpret_cast<void*>(cfml::cf_atan));
    llvm::sys::DynamicLibrary::AddSymbol("cf_atan2", reinterpret_cast<void*>(cfml::cf_atan2));
    llvm::sys::DynamicLibrary::AddSymbol("cf_binarydecode", reinterpret_cast<void*>(cfml::cf_binarydecode));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ceiling", reinterpret_cast<void*>(cfml::cf_ceiling));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cos", reinterpret_cast<void*>(cfml::cf_cos));
    llvm::sys::DynamicLibrary::AddSymbol("cf_exp", reinterpret_cast<void*>(cfml::cf_exp));
    llvm::sys::DynamicLibrary::AddSymbol("cf_floor", reinterpret_cast<void*>(cfml::cf_floor));
    llvm::sys::DynamicLibrary::AddSymbol("cf_incrementvalue", reinterpret_cast<void*>(cfml::cf_incrementvalue));
    llvm::sys::DynamicLibrary::AddSymbol("cf_decrementvalue", reinterpret_cast<void*>(cfml::cf_decrementvalue));
    llvm::sys::DynamicLibrary::AddSymbol("cf_int", reinterpret_cast<void*>(cfml::cf_int));
    llvm::sys::DynamicLibrary::AddSymbol("cf_log", reinterpret_cast<void*>(cfml::cf_log));
    llvm::sys::DynamicLibrary::AddSymbol("cf_log10", reinterpret_cast<void*>(cfml::cf_log10));
    llvm::sys::DynamicLibrary::AddSymbol("cf_max", reinterpret_cast<void*>(cfml::cf_max));
    llvm::sys::DynamicLibrary::AddSymbol("cf_min", reinterpret_cast<void*>(cfml::cf_min));
    llvm::sys::DynamicLibrary::AddSymbol("cf_pi", reinterpret_cast<void*>(cfml::cf_pi));
    llvm::sys::DynamicLibrary::AddSymbol("cf_rand", reinterpret_cast<void*>(cfml::cf_rand));
    llvm::sys::DynamicLibrary::AddSymbol("cf_randomize", reinterpret_cast<void*>(cfml::cf_randomize));
    llvm::sys::DynamicLibrary::AddSymbol("cf_round", reinterpret_cast<void*>(cfml::cf_round));
    llvm::sys::DynamicLibrary::AddSymbol("cf_sgn", reinterpret_cast<void*>(cfml::cf_sgn));
    llvm::sys::DynamicLibrary::AddSymbol("cf_sin", reinterpret_cast<void*>(cfml::cf_sin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_sqr", reinterpret_cast<void*>(cfml::cf_sqr));
    llvm::sys::DynamicLibrary::AddSymbol("cf_tan", reinterpret_cast<void*>(cfml::cf_tan));
    llvm::sys::DynamicLibrary::AddSymbol("cf_len", reinterpret_cast<void*>(cfml::cf_len));
    llvm::sys::DynamicLibrary::AddSymbol("cf_left", reinterpret_cast<void*>(cfml::cf_left));
    llvm::sys::DynamicLibrary::AddSymbol("cf_right", reinterpret_cast<void*>(cfml::cf_right));
    llvm::sys::DynamicLibrary::AddSymbol("cf_mid", reinterpret_cast<void*>(cfml::cf_mid));
    llvm::sys::DynamicLibrary::AddSymbol("cf_trim", reinterpret_cast<void*>(cfml::cf_trim));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ltrim", reinterpret_cast<void*>(cfml::cf_ltrim));
    llvm::sys::DynamicLibrary::AddSymbol("cf_rtrim", reinterpret_cast<void*>(cfml::cf_rtrim));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lcase", reinterpret_cast<void*>(cfml::cf_lcase));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ucase", reinterpret_cast<void*>(cfml::cf_ucase));
    llvm::sys::DynamicLibrary::AddSymbol("cf_reverse", reinterpret_cast<void*>(cfml::cf_reverse));
    llvm::sys::DynamicLibrary::AddSymbol("cf_repeatstring", reinterpret_cast<void*>(cfml::cf_repeatstring));
    llvm::sys::DynamicLibrary::AddSymbol("cf_replace", reinterpret_cast<void*>(cfml::cf_replace));
    llvm::sys::DynamicLibrary::AddSymbol("cf_replacenocase", reinterpret_cast<void*>(cfml::cf_replacenocase));
    llvm::sys::DynamicLibrary::AddSymbol("cf_find", reinterpret_cast<void*>(cfml::cf_find));
    llvm::sys::DynamicLibrary::AddSymbol("cf_findnocase", reinterpret_cast<void*>(cfml::cf_findnocase));
    llvm::sys::DynamicLibrary::AddSymbol("cf_compare", reinterpret_cast<void*>(cfml::cf_compare));
    llvm::sys::DynamicLibrary::AddSymbol("cf_comparenocase", reinterpret_cast<void*>(cfml::cf_comparenocase));
    llvm::sys::DynamicLibrary::AddSymbol("cf_decimalformat", reinterpret_cast<void*>(cfml::cf_decimalformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_dollarformat", reinterpret_cast<void*>(cfml::cf_dollarformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_yesnoformat", reinterpret_cast<void*>(cfml::cf_yesnoformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_now", reinterpret_cast<void*>(cfml::cf_now));
    llvm::sys::DynamicLibrary::AddSymbol("cf_createdatetime", reinterpret_cast<void*>(cfml::cf_createdatetime));
    llvm::sys::DynamicLibrary::AddSymbol("cf_createdate", reinterpret_cast<void*>(cfml::cf_createdate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_createtime", reinterpret_cast<void*>(cfml::cf_createtime));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isdate", reinterpret_cast<void*>(cfml::cf_isdate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_year", reinterpret_cast<void*>(cfml::cf_year));
    llvm::sys::DynamicLibrary::AddSymbol("cf_month", reinterpret_cast<void*>(cfml::cf_month));
    llvm::sys::DynamicLibrary::AddSymbol("cf_day", reinterpret_cast<void*>(cfml::cf_day));
    llvm::sys::DynamicLibrary::AddSymbol("cf_hour", reinterpret_cast<void*>(cfml::cf_hour));
    llvm::sys::DynamicLibrary::AddSymbol("cf_minute", reinterpret_cast<void*>(cfml::cf_minute));
    llvm::sys::DynamicLibrary::AddSymbol("cf_second", reinterpret_cast<void*>(cfml::cf_second));

    // Compiler-extension functions (the `__` prefix family, reserved like C's
    // `__` identifiers): direct-call symbols for cf___<name>. Implementations
    // in src/cffunctions/fn_<name>.cpp. The codegen dispatches any `__foo()`
    // call to cf___foo(args, argc) at compile time.
    llvm::sys::DynamicLibrary::AddSymbol("cf___configget", reinterpret_cast<void*>(cfml::cf___configget));
    llvm::sys::DynamicLibrary::AddSymbol("cf___configset", reinterpret_cast<void*>(cfml::cf___configset));
    llvm::sys::DynamicLibrary::AddSymbol("cf___datasourcetest", reinterpret_cast<void*>(cfml::cf___datasourcetest));
    llvm::sys::DynamicLibrary::AddSymbol("cf___serverinfo", reinterpret_cast<void*>(cfml::cf___serverinfo));
    llvm::sys::DynamicLibrary::AddSymbol("cf___configreset", reinterpret_cast<void*>(cfml::cf___configreset));
    llvm::sys::DynamicLibrary::AddSymbol("cf___cacheinfo", reinterpret_cast<void*>(cfml::cf___cacheinfo));
    llvm::sys::DynamicLibrary::AddSymbol("cf___cacheevict", reinterpret_cast<void*>(cfml::cf___cacheevict));
    llvm::sys::DynamicLibrary::AddSymbol("cf___cacheclear", reinterpret_cast<void*>(cfml::cf___cacheclear));



    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_get_var", reinterpret_cast<void*>(cfml::cfvariant_get_var));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_get_var_fast", reinterpret_cast<void*>(cfml::cfvariant_get_var_fast));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_get_member", reinterpret_cast<void*>(cfml::cfvariant_get_member));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_bare_identifier", reinterpret_cast<void*>(cfml::cfvariant_bare_identifier));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_bare_identifier_fast", reinterpret_cast<void*>(cfml::cfvariant_bare_identifier_fast));
    llvm::sys::DynamicLibrary::AddSymbol("cf_application_enable", reinterpret_cast<void*>(cfml::cf_application_enable));
    llvm::sys::DynamicLibrary::AddSymbol("cf_set_search_implicit_scopes", reinterpret_cast<void*>(cfml::cf_set_search_implicit_scopes));
    llvm::sys::DynamicLibrary::AddSymbol("cf_param", reinterpret_cast<void*>(cfml::cf_param));
    llvm::sys::DynamicLibrary::AddSymbol("cf_objectcache", reinterpret_cast<void*>(cfml::cf_objectcache));
    llvm::sys::DynamicLibrary::AddSymbol("cf_include", reinterpret_cast<void*>(cfml::cf_include));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cferror_register", reinterpret_cast<void*>(cfml::cf_cferror_register));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cache_tag_begin", reinterpret_cast<void*>(cfml::cf_cache_tag_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cache_tag_end", reinterpret_cast<void*>(cfml::cf_cache_tag_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cache_store_page", reinterpret_cast<void*>(cfml::cf_cache_store_page));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cache_reset", reinterpret_cast<void*>(cfml::cf_cache_reset));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_assign", reinterpret_cast<void*>(cfml::cfvariant_assign));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_add", reinterpret_cast<void*>(cfml::cfvariant_add));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_sub", reinterpret_cast<void*>(cfml::cfvariant_sub));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_mul", reinterpret_cast<void*>(cfml::cfvariant_mul));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_div", reinterpret_cast<void*>(cfml::cfvariant_div));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_mod", reinterpret_cast<void*>(cfml::cfvariant_mod));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_idiv", reinterpret_cast<void*>(cfml::cfvariant_idiv));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_pow", reinterpret_cast<void*>(cfml::cfvariant_pow));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_neg", reinterpret_cast<void*>(cfml::cfvariant_neg));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_concat", reinterpret_cast<void*>(cfml::cfvariant_concat));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_and", reinterpret_cast<void*>(cfml::cfvariant_and));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_or", reinterpret_cast<void*>(cfml::cfvariant_or));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_xor", reinterpret_cast<void*>(cfml::cfvariant_xor));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_not", reinterpret_cast<void*>(cfml::cfvariant_not));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_compare", reinterpret_cast<void*>(cfml::cfvariant_compare));
    llvm::sys::DynamicLibrary::AddSymbol("cf_is_truthy_value", reinterpret_cast<void*>(cfml::cf_is_truthy_value));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_index", reinterpret_cast<void*>(cfml::cfvariant_index));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_index_named", reinterpret_cast<void*>(cfml::cfvariant_index_named));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_index_assign", reinterpret_cast<void*>(cfml::cfvariant_index_assign));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_index_assign_deep", reinterpret_cast<void*>(cfml::cfvariant_index_assign_deep));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_call_function", reinterpret_cast<void*>(cfml::cfvariant_call_function));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_member_method", reinterpret_cast<void*>(cfml::cfvariant_member_method));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_is_truthy", reinterpret_cast<void*>(cfml::cfvariant_is_truthy));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_create_udf", reinterpret_cast<void*>(cfml::cfvariant_create_udf));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_begin", reinterpret_cast<void*>(cfml::cf_udf_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_mark_local", reinterpret_cast<void*>(cfml::cf_udf_mark_local));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_end", reinterpret_cast<void*>(cfml::cf_udf_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_invoke", reinterpret_cast<void*>(cfml::cf_udf_invoke));
    llvm::sys::DynamicLibrary::AddSymbol("cf_named_args_marker", reinterpret_cast<void*>(cfml::cf_named_args_marker));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_type", reinterpret_cast<void*>(cfml::cfvariant_type));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_register_temp", reinterpret_cast<void*>(cfml::cf_udf_register_temp));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_build_arguments", reinterpret_cast<void*>(cfml::cf_udf_build_arguments));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_remove_params", reinterpret_cast<void*>(cfml::cf_udf_remove_params));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_args_set_or_null", reinterpret_cast<void*>(cfml::cf_udf_args_set_or_null));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_args_metadata", reinterpret_cast<void*>(cfml::cf_udf_args_metadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_coerce_arg", reinterpret_cast<void*>(cfml::cf_udf_coerce_arg));
    llvm::sys::DynamicLibrary::AddSymbol("cf_udf_coerce_return", reinterpret_cast<void*>(cfml::cf_udf_coerce_return));
    llvm::sys::DynamicLibrary::AddSymbol("cf_throw_missing_argument", reinterpret_cast<void*>(cfml::cf_throw_missing_argument));

    // ColdFusion Component (CFC) runtime helpers.
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_udf_begin", reinterpret_cast<void*>(cfml::cf_component_udf_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_instantiate", reinterpret_cast<void*>(cfml::cf_component_instantiate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_load", reinterpret_cast<void*>(cfml::cf_component_load));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_invoke", reinterpret_cast<void*>(cfml::cf_component_invoke));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_invoke_instance", reinterpret_cast<void*>(cfml::cf_component_invoke_instance));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_method_handle", reinterpret_cast<void*>(cfml::cf_component_method_handle));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_method_handle_invoke", reinterpret_cast<void*>(cfml::cf_component_method_handle_invoke));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_has_method", reinterpret_cast<void*>(cfml::cf_component_has_method));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_has_method_on", reinterpret_cast<void*>(cfml::cf_component_has_method_on));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_throw_method_not_found", reinterpret_cast<void*>(cfml::cf_component_throw_method_not_found));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_new", reinterpret_cast<void*>(cfml::cf_component_new));
    llvm::sys::DynamicLibrary::AddSymbol("cf_component_get_super_scope", reinterpret_cast<void*>(cfml::cf_component_get_super_scope));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cfobject", reinterpret_cast<void*>(cfml::cf_cfobject));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cfinvoke", reinterpret_cast<void*>(cfml::cf_cfinvoke));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cfinvoke_begin", reinterpret_cast<void*>(cfml::cf_cfinvoke_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cfinvoke_argument", reinterpret_cast<void*>(cfml::cf_cfinvoke_argument));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cfinvoke_end", reinterpret_cast<void*>(cfml::cf_cfinvoke_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_import_path", reinterpret_cast<void*>(cfml::cf_import_path));
    llvm::sys::DynamicLibrary::AddSymbol("cf_import_taglib", reinterpret_cast<void*>(cfml::cf_import_taglib));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cfmodule", reinterpret_cast<void*>(cfml::cf_cfmodule));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cfassociate", reinterpret_cast<void*>(cfml::cf_cfassociate));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_to_int", reinterpret_cast<void*>(cfml::cfvariant_to_int));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_to_long", reinterpret_cast<void*>(cfml::cfvariant_to_long));
    llvm::sys::DynamicLibrary::AddSymbol("cfforin_length", reinterpret_cast<void*>(cfml::cfforInLength));
    llvm::sys::DynamicLibrary::AddSymbol("cfforin_item", reinterpret_cast<void*>(cfml::cfforInItem));
    llvm::sys::DynamicLibrary::AddSymbol("cfvariant_copy_value", reinterpret_cast<void*>(cfml::cfvariant_copy_value));

    // Unimplemented function JIT symbol registrations
    llvm::sys::DynamicLibrary::AddSymbol("cf_addsoaprequestheader", reinterpret_cast<void*>(cfml::cf_addsoaprequestheader));
    llvm::sys::DynamicLibrary::AddSymbol("cf_addsoapresponseheader", reinterpret_cast<void*>(cfml::cf_addsoapresponseheader));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ajaxlink", reinterpret_cast<void*>(cfml::cf_ajaxlink));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ajaxonload", reinterpret_cast<void*>(cfml::cf_ajaxonload));
    llvm::sys::DynamicLibrary::AddSymbol("cf_applicationstop", reinterpret_cast<void*>(cfml::cf_applicationstop));
    llvm::sys::DynamicLibrary::AddSymbol("cf_authenticatedcontext", reinterpret_cast<void*>(cfml::cf_authenticatedcontext));
    llvm::sys::DynamicLibrary::AddSymbol("cf_authenticateduser", reinterpret_cast<void*>(cfml::cf_authenticateduser));
    llvm::sys::DynamicLibrary::AddSymbol("cf_binaryencode", reinterpret_cast<void*>(cfml::cf_binaryencode));
    llvm::sys::DynamicLibrary::AddSymbol("cf_bitand", reinterpret_cast<void*>(cfml::cf_bitand));
    llvm::sys::DynamicLibrary::AddSymbol("cf_bitmaskclear", reinterpret_cast<void*>(cfml::cf_bitmaskclear));
    llvm::sys::DynamicLibrary::AddSymbol("cf_bitmaskread", reinterpret_cast<void*>(cfml::cf_bitmaskread));
    llvm::sys::DynamicLibrary::AddSymbol("cf_bitmaskset", reinterpret_cast<void*>(cfml::cf_bitmaskset));
    llvm::sys::DynamicLibrary::AddSymbol("cf_bitnot", reinterpret_cast<void*>(cfml::cf_bitnot));
    llvm::sys::DynamicLibrary::AddSymbol("cf_bitor", reinterpret_cast<void*>(cfml::cf_bitor));
    llvm::sys::DynamicLibrary::AddSymbol("cf_bitshln", reinterpret_cast<void*>(cfml::cf_bitshln));
    llvm::sys::DynamicLibrary::AddSymbol("cf_bitshrn", reinterpret_cast<void*>(cfml::cf_bitshrn));
    llvm::sys::DynamicLibrary::AddSymbol("cf_bitxor", reinterpret_cast<void*>(cfml::cf_bitxor));
    llvm::sys::DynamicLibrary::AddSymbol("cf_booleanformat", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*)>(cfml::cf_booleanformat)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cacheget", reinterpret_cast<void*>(cfml::cf_cacheget));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cachegetallids", reinterpret_cast<void*>(cfml::cf_cachegetallids));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cachegetmetadata", reinterpret_cast<void*>(cfml::cf_cachegetmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cachegetproperties", reinterpret_cast<void*>(cfml::cf_cachegetproperties));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cachegetsession", reinterpret_cast<void*>(cfml::cf_cachegetsession));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cacheidexists", reinterpret_cast<void*>(cfml::cf_cacheidexists));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cacheput", reinterpret_cast<void*>(cfml::cf_cacheput));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cacheregionexists", reinterpret_cast<void*>(cfml::cf_cacheregionexists));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cacheregionnew", reinterpret_cast<void*>(cfml::cf_cacheregionnew));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cacheregionremove", reinterpret_cast<void*>(cfml::cf_cacheregionremove));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cacheremove", reinterpret_cast<void*>(cfml::cf_cacheremove));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cacheremoveall", reinterpret_cast<void*>(cfml::cf_cacheremoveall));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cachesetproperties", reinterpret_cast<void*>(cfml::cf_cachesetproperties));
    llvm::sys::DynamicLibrary::AddSymbol("cf_callstackdump", reinterpret_cast<void*>(cfml::cf_callstackdump));
    llvm::sys::DynamicLibrary::AddSymbol("cf_callstackget", reinterpret_cast<void*>(cfml::cf_callstackget));
    llvm::sys::DynamicLibrary::AddSymbol("cf_candeserialize", reinterpret_cast<void*>(cfml::cf_candeserialize));
    llvm::sys::DynamicLibrary::AddSymbol("cf_canonicalize", reinterpret_cast<void*>(cfml::cf_canonicalize));
    llvm::sys::DynamicLibrary::AddSymbol("cf_canserialize", reinterpret_cast<void*>(cfml::cf_canserialize));
    llvm::sys::DynamicLibrary::AddSymbol("cf_charsetdecode", reinterpret_cast<void*>(cfml::cf_charsetdecode));
    llvm::sys::DynamicLibrary::AddSymbol("cf_charsetencode", reinterpret_cast<void*>(cfml::cf_charsetencode));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cjustify", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_cjustify)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_createencryptedjwt", reinterpret_cast<void*>(cfml::cf_createencryptedjwt));
    llvm::sys::DynamicLibrary::AddSymbol("cf_createobject", reinterpret_cast<void*>(cfml::cf_createobject));
    llvm::sys::DynamicLibrary::AddSymbol("cf_createodbcdate", reinterpret_cast<void*>(cfml::cf_createodbcdate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_createodbcdatetime", reinterpret_cast<void*>(cfml::cf_createodbcdatetime));
    llvm::sys::DynamicLibrary::AddSymbol("cf_createodbctime", reinterpret_cast<void*>(cfml::cf_createodbctime));
    llvm::sys::DynamicLibrary::AddSymbol("cf_createsignedjwt", reinterpret_cast<void*>(cfml::cf_createsignedjwt));
    llvm::sys::DynamicLibrary::AddSymbol("cf_createtimespan", reinterpret_cast<void*>(cfml::cf_createtimespan));
    llvm::sys::DynamicLibrary::AddSymbol("cf_csrfgeneratetoken", reinterpret_cast<void*>(cfml::cf_csrfgeneratetoken));
    llvm::sys::DynamicLibrary::AddSymbol("cf_csrfverifytoken", reinterpret_cast<void*>(cfml::cf_csrfverifytoken));
    llvm::sys::DynamicLibrary::AddSymbol("cf_csvprocess", reinterpret_cast<void*>(cfml::cf_csvprocess));
    llvm::sys::DynamicLibrary::AddSymbol("cf_csvread", reinterpret_cast<void*>(cfml::cf_csvread));
    llvm::sys::DynamicLibrary::AddSymbol("cf_csvwrite", reinterpret_cast<void*>(cfml::cf_csvwrite));
    llvm::sys::DynamicLibrary::AddSymbol("cf_dateadd", reinterpret_cast<void*>(cfml::cf_dateadd));
    llvm::sys::DynamicLibrary::AddSymbol("cf_datecompare", reinterpret_cast<void*>(cfml::cf_datecompare));
    llvm::sys::DynamicLibrary::AddSymbol("cf_dateconvert", reinterpret_cast<void*>(cfml::cf_dateconvert));
    llvm::sys::DynamicLibrary::AddSymbol("cf_datediff", reinterpret_cast<void*>(cfml::cf_datediff));
    llvm::sys::DynamicLibrary::AddSymbol("cf_datepart", reinterpret_cast<void*>(cfml::cf_datepart));
    llvm::sys::DynamicLibrary::AddSymbol("cf_dayofweek", reinterpret_cast<void*>(cfml::cf_dayofweek));
    llvm::sys::DynamicLibrary::AddSymbol("cf_dayofweekasstring", reinterpret_cast<void*>(cfml::cf_dayofweekasstring));
    llvm::sys::DynamicLibrary::AddSymbol("cf_dayofyear", reinterpret_cast<void*>(cfml::cf_dayofyear));
    llvm::sys::DynamicLibrary::AddSymbol("cf_daysinmonth", reinterpret_cast<void*>(cfml::cf_daysinmonth));
    llvm::sys::DynamicLibrary::AddSymbol("cf_daysinyear", reinterpret_cast<void*>(cfml::cf_daysinyear));
    llvm::sys::DynamicLibrary::AddSymbol("cf_de", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*)>(cfml::cf_de)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_decodeforhtml", reinterpret_cast<void*>(cfml::cf_decodeforhtml));
    llvm::sys::DynamicLibrary::AddSymbol("cf_decodefromurl", reinterpret_cast<void*>(cfml::cf_decodefromurl));
    llvm::sys::DynamicLibrary::AddSymbol("cf_decrypt", reinterpret_cast<void*>(cfml::cf_decrypt));
    llvm::sys::DynamicLibrary::AddSymbol("cf_decryptbinary", reinterpret_cast<void*>(cfml::cf_decryptbinary));
    llvm::sys::DynamicLibrary::AddSymbol("cf_deleteclientvariable", reinterpret_cast<void*>(cfml::cf_deleteclientvariable));
    llvm::sys::DynamicLibrary::AddSymbol("cf_deserialize", reinterpret_cast<void*>(cfml::cf_deserialize));
    llvm::sys::DynamicLibrary::AddSymbol("cf_deserializejson", reinterpret_cast<void*>(cfml::cf_deserializejson));
    llvm::sys::DynamicLibrary::AddSymbol("cf_deserializexml", reinterpret_cast<void*>(cfml::cf_deserializexml));
    llvm::sys::DynamicLibrary::AddSymbol("cf_directorycopy", reinterpret_cast<void*>(cfml::cf_directorycopy));
    llvm::sys::DynamicLibrary::AddSymbol("cf_directorylist", reinterpret_cast<void*>(cfml::cf_directorylist));
    llvm::sys::DynamicLibrary::AddSymbol("cf_directoryrename", reinterpret_cast<void*>(cfml::cf_directoryrename));
    llvm::sys::DynamicLibrary::AddSymbol("cf_dotnettocftype", reinterpret_cast<void*>(cfml::cf_dotnettocftype));
    llvm::sys::DynamicLibrary::AddSymbol("cf_duplicate", reinterpret_cast<void*>(cfml::cf_duplicate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encodeforcss", reinterpret_cast<void*>(cfml::cf_encodeforcss));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encodefordn", reinterpret_cast<void*>(cfml::cf_encodefordn));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encodeforhtml", reinterpret_cast<void*>(cfml::cf_encodeforhtml));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encodeforhtmlattribute", reinterpret_cast<void*>(cfml::cf_encodeforhtmlattribute));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encodeforjavascript", reinterpret_cast<void*>(cfml::cf_encodeforjavascript));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encodeforldap", reinterpret_cast<void*>(cfml::cf_encodeforldap));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encodeforurl", reinterpret_cast<void*>(cfml::cf_encodeforurl));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encodeforxml", reinterpret_cast<void*>(cfml::cf_encodeforxml));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encodeforxmlattribute", reinterpret_cast<void*>(cfml::cf_encodeforxmlattribute));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encodeforxpath", reinterpret_cast<void*>(cfml::cf_encodeforxpath));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encrypt", reinterpret_cast<void*>(cfml::cf_encrypt));
    llvm::sys::DynamicLibrary::AddSymbol("cf_encryptbinary", reinterpret_cast<void*>(cfml::cf_encryptbinary));
    llvm::sys::DynamicLibrary::AddSymbol("cf_entitydelete", reinterpret_cast<void*>(cfml::cf_entitydelete));
    llvm::sys::DynamicLibrary::AddSymbol("cf_entityload", reinterpret_cast<void*>(cfml::cf_entityload));
    llvm::sys::DynamicLibrary::AddSymbol("cf_entityloadbyexample", reinterpret_cast<void*>(cfml::cf_entityloadbyexample));
    llvm::sys::DynamicLibrary::AddSymbol("cf_entityloadbypk", reinterpret_cast<void*>(cfml::cf_entityloadbypk));
    llvm::sys::DynamicLibrary::AddSymbol("cf_entitymerge", reinterpret_cast<void*>(cfml::cf_entitymerge));
    llvm::sys::DynamicLibrary::AddSymbol("cf_entitynew", reinterpret_cast<void*>(cfml::cf_entitynew));
    llvm::sys::DynamicLibrary::AddSymbol("cf_entityreload", reinterpret_cast<void*>(cfml::cf_entityreload));
    llvm::sys::DynamicLibrary::AddSymbol("cf_entitysave", reinterpret_cast<void*>(cfml::cf_entitysave));
    llvm::sys::DynamicLibrary::AddSymbol("cf_entitytoquery", reinterpret_cast<void*>(cfml::cf_entitytoquery));
    llvm::sys::DynamicLibrary::AddSymbol("cf_evaluate", reinterpret_cast<void*>(cfml::cf_evaluate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_expandpath", reinterpret_cast<void*>(cfml::cf_expandpath));
    llvm::sys::DynamicLibrary::AddSymbol("cf_fileclose", reinterpret_cast<void*>(cfml::cf_fileclose));
    llvm::sys::DynamicLibrary::AddSymbol("cf_filegetmimetype", reinterpret_cast<void*>(cfml::cf_filegetmimetype));
    llvm::sys::DynamicLibrary::AddSymbol("cf_fileiseof", reinterpret_cast<void*>(cfml::cf_fileiseof));
    llvm::sys::DynamicLibrary::AddSymbol("cf_fileopen", reinterpret_cast<void*>(cfml::cf_fileopen));
    llvm::sys::DynamicLibrary::AddSymbol("cf_filereadbinary", reinterpret_cast<void*>(cfml::cf_filereadbinary));
    llvm::sys::DynamicLibrary::AddSymbol("cf_filereadline", reinterpret_cast<void*>(cfml::cf_filereadline));
    llvm::sys::DynamicLibrary::AddSymbol("cf_fileseek", reinterpret_cast<void*>(cfml::cf_fileseek));
    llvm::sys::DynamicLibrary::AddSymbol("cf_filesetaccessmode", reinterpret_cast<void*>(cfml::cf_filesetaccessmode));
    llvm::sys::DynamicLibrary::AddSymbol("cf_filesetattribute", reinterpret_cast<void*>(cfml::cf_filesetattribute));
    llvm::sys::DynamicLibrary::AddSymbol("cf_filesetlastmodified", reinterpret_cast<void*>(cfml::cf_filesetlastmodified));
    llvm::sys::DynamicLibrary::AddSymbol("cf_fileskipbytes", reinterpret_cast<void*>(cfml::cf_fileskipbytes));
    llvm::sys::DynamicLibrary::AddSymbol("cf_fileupload", reinterpret_cast<void*>(cfml::cf_fileupload));
    llvm::sys::DynamicLibrary::AddSymbol("cf_fileuploadall", reinterpret_cast<void*>(cfml::cf_fileuploadall));
    llvm::sys::DynamicLibrary::AddSymbol("cf_filewriteline", reinterpret_cast<void*>(cfml::cf_filewriteline));
    llvm::sys::DynamicLibrary::AddSymbol("cf_findoneof", reinterpret_cast<void*>(cfml::cf_findoneof));
    llvm::sys::DynamicLibrary::AddSymbol("cf_firstdayofmonth", reinterpret_cast<void*>(cfml::cf_firstdayofmonth));
    llvm::sys::DynamicLibrary::AddSymbol("cf_fix", reinterpret_cast<void*>(cfml::cf_fix));
    llvm::sys::DynamicLibrary::AddSymbol("cf_formatbasen", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_formatbasen)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_generate3deskey", reinterpret_cast<void*>(cfml::cf_generate3deskey));
    llvm::sys::DynamicLibrary::AddSymbol("cf_generatepbkdfkey", reinterpret_cast<void*>(cfml::cf_generatepbkdfkey));
    llvm::sys::DynamicLibrary::AddSymbol("cf_generatesamlspmetadata", reinterpret_cast<void*>(cfml::cf_generatesamlspmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_generatesecretkey", reinterpret_cast<void*>(cfml::cf_generatesecretkey));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getapplicationmetadata", reinterpret_cast<void*>(cfml::cf_getapplicationmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getauthuser", reinterpret_cast<void*>(cfml::cf_getauthuser));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getbasetagdata", reinterpret_cast<void*>(cfml::cf_getbasetagdata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getbasetaglist", reinterpret_cast<void*>(cfml::cf_getbasetaglist));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getclientvariableslist", reinterpret_cast<void*>(cfml::cf_getclientvariableslist));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getcomponentmetadata", reinterpret_cast<void*>(cfml::cf_getcomponentmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getcontextroot", reinterpret_cast<void*>(cfml::cf_getcontextroot));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getcpuusage", reinterpret_cast<void*>(cfml::cf_getcpuusage));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getcspnonce", reinterpret_cast<void*>(cfml::cf_getcspnonce));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getencoding", reinterpret_cast<void*>(cfml::cf_getencoding));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getexception", reinterpret_cast<void*>(cfml::cf_getexception));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getfreespace", reinterpret_cast<void*>(cfml::cf_getfreespace));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getfunctioncalledname", reinterpret_cast<void*>(cfml::cf_getfunctioncalledname));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getfunctionlist", reinterpret_cast<void*>(cfml::cf_getfunctionlist));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getgatewayhelper", reinterpret_cast<void*>(cfml::cf_getgatewayhelper));
    llvm::sys::DynamicLibrary::AddSymbol("cf_gethttprequestdata", reinterpret_cast<void*>(cfml::cf_gethttprequestdata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_gethttptimestring", reinterpret_cast<void*>(cfml::cf_gethttptimestring));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getk2serverdoccount", reinterpret_cast<void*>(cfml::cf_getk2serverdoccount));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getk2serverdoccountlimit", reinterpret_cast<void*>(cfml::cf_getk2serverdoccountlimit));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getlocaledisplayname", reinterpret_cast<void*>(cfml::cf_getlocaledisplayname));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getlocalhostip", reinterpret_cast<void*>(cfml::cf_getlocalhostip));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getmetadata", reinterpret_cast<void*>(cfml::cf_getmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getmetricdata", reinterpret_cast<void*>(cfml::cf_getmetricdata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getpagecontext", reinterpret_cast<void*>(cfml::cf_getpagecontext));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getprinterinfo", reinterpret_cast<void*>(cfml::cf_getprinterinfo));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getprinterlist", reinterpret_cast<void*>(cfml::cf_getprinterlist));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getprofilesections", reinterpret_cast<void*>(cfml::cf_getprofilesections));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getprofilestring", reinterpret_cast<void*>(cfml::cf_getprofilestring));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getpropertyfile", reinterpret_cast<void*>(cfml::cf_getpropertyfile));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getpropertystring", reinterpret_cast<void*>(cfml::cf_getpropertystring));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getreadableimageformats", reinterpret_cast<void*>(cfml::cf_getreadableimageformats));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getsafehtml", reinterpret_cast<void*>(cfml::cf_getsafehtml));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getsamlauthrequest", reinterpret_cast<void*>(cfml::cf_getsamlauthrequest));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getsamllogoutrequest", reinterpret_cast<void*>(cfml::cf_getsamllogoutrequest));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getsoaprequest", reinterpret_cast<void*>(cfml::cf_getsoaprequest));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getsoaprequestheader", reinterpret_cast<void*>(cfml::cf_getsoaprequestheader));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getsoapresponse", reinterpret_cast<void*>(cfml::cf_getsoapresponse));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getsoapresponseheader", reinterpret_cast<void*>(cfml::cf_getsoapresponseheader));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getsystemfreememory", reinterpret_cast<void*>(cfml::cf_getsystemfreememory));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getsystemtotalmemory", reinterpret_cast<void*>(cfml::cf_getsystemtotalmemory));
    llvm::sys::DynamicLibrary::AddSymbol("cf_gettimezoneinfo", reinterpret_cast<void*>(cfml::cf_gettimezoneinfo));
    llvm::sys::DynamicLibrary::AddSymbol("cf_gettoken", reinterpret_cast<void*>(cfml::cf_gettoken));
    llvm::sys::DynamicLibrary::AddSymbol("cf_gettotalspace", reinterpret_cast<void*>(cfml::cf_gettotalspace));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getuserroles", reinterpret_cast<void*>(cfml::cf_getuserroles));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getvfsmetadata", reinterpret_cast<void*>(cfml::cf_getvfsmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_getwriteableimageformats", reinterpret_cast<void*>(cfml::cf_getwriteableimageformats));
    llvm::sys::DynamicLibrary::AddSymbol("cf_hash", reinterpret_cast<void*>(cfml::cf_hash));
    llvm::sys::DynamicLibrary::AddSymbol("cf_hmac", reinterpret_cast<void*>(cfml::cf_hmac));
    llvm::sys::DynamicLibrary::AddSymbol("cf_hqlmethods", reinterpret_cast<void*>(cfml::cf_hqlmethods));
    llvm::sys::DynamicLibrary::AddSymbol("cf_htmlcodeformat", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_htmlcodeformat)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_htmleditformat", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_htmleditformat)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_iif", reinterpret_cast<void*>(cfml::cf_iif));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imageaddborder", reinterpret_cast<void*>(cfml::cf_imageaddborder));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imageblur", reinterpret_cast<void*>(cfml::cf_imageblur));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imageclearrect", reinterpret_cast<void*>(cfml::cf_imageclearrect));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagecopy", reinterpret_cast<void*>(cfml::cf_imagecopy));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagecreatecaptcha", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*)>(cfml::cf_imagecreatecaptcha)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagecrop", reinterpret_cast<void*>(cfml::cf_imagecrop));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawarc", reinterpret_cast<void*>(cfml::cf_imagedrawarc));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawbeveledrect", reinterpret_cast<void*>(cfml::cf_imagedrawbeveledrect));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawcubiccurve", reinterpret_cast<void*>(cfml::cf_imagedrawcubiccurve));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawline", reinterpret_cast<void*>(cfml::cf_imagedrawline));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawlines", reinterpret_cast<void*>(cfml::cf_imagedrawlines));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawoval", reinterpret_cast<void*>(cfml::cf_imagedrawoval));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawpoint", reinterpret_cast<void*>(cfml::cf_imagedrawpoint));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawquadraticcurve", reinterpret_cast<void*>(cfml::cf_imagedrawquadraticcurve));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawrect", reinterpret_cast<void*>(cfml::cf_imagedrawrect));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawroundrect", reinterpret_cast<void*>(cfml::cf_imagedrawroundrect));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagedrawtext", reinterpret_cast<void*>(cfml::cf_imagedrawtext));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imageflip", reinterpret_cast<void*>(cfml::cf_imageflip));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagegetblob", reinterpret_cast<void*>(cfml::cf_imagegetblob));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagegetbufferedimage", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*)>(cfml::cf_imagegetbufferedimage)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagegetexifmetadata", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*)>(cfml::cf_imagegetexifmetadata)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagegetexiftag", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_imagegetexiftag)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagegetheight", reinterpret_cast<void*>(cfml::cf_imagegetheight));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagegetiptcmetadata", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*)>(cfml::cf_imagegetiptcmetadata)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagegetiptctag", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_imagegetiptctag)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagegetmetadata", reinterpret_cast<void*>(cfml::cf_imagegetmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagegetwidth", reinterpret_cast<void*>(cfml::cf_imagegetwidth));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagegrayscale", reinterpret_cast<void*>(cfml::cf_imagegrayscale));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imageinfo", reinterpret_cast<void*>(cfml::cf_imageinfo));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagemakecolortransparent", reinterpret_cast<void*>(cfml::cf_imagemakecolortransparent));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagemaketranslucent", reinterpret_cast<void*>(cfml::cf_imagemaketranslucent));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagenegative", reinterpret_cast<void*>(cfml::cf_imagenegative));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagenew", reinterpret_cast<void*>(cfml::cf_imagenew));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imageoverlay", reinterpret_cast<void*>(cfml::cf_imageoverlay));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagepaste", reinterpret_cast<void*>(cfml::cf_imagepaste));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imageread", reinterpret_cast<void*>(cfml::cf_imageread));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagereadbase64", reinterpret_cast<void*>(cfml::cf_imagereadbase64));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imageresize", reinterpret_cast<void*>(cfml::cf_imageresize));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagerotate", reinterpret_cast<void*>(cfml::cf_imagerotate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagerotatedrawingaxis", reinterpret_cast<void*>(cfml::cf_imagerotatedrawingaxis));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagescaletofit", reinterpret_cast<void*>(cfml::cf_imagescaletofit));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagesetantialiasing", reinterpret_cast<void*>(cfml::cf_imagesetantialiasing));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagesetbackgroundcolor", reinterpret_cast<void*>(cfml::cf_imagesetbackgroundcolor));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagesetdrawingcolor", reinterpret_cast<void*>(cfml::cf_imagesetdrawingcolor));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagesetdrawingstroke", reinterpret_cast<void*>(cfml::cf_imagesetdrawingstroke));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagesetdrawingtransparency", reinterpret_cast<void*>(cfml::cf_imagesetdrawingtransparency));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagesharpen", reinterpret_cast<void*>(cfml::cf_imagesharpen));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imageshear", reinterpret_cast<void*>(cfml::cf_imageshear));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagesheardrawingaxis", reinterpret_cast<void*>(cfml::cf_imagesheardrawingaxis));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagetranslate", reinterpret_cast<void*>(cfml::cf_imagetranslate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagetranslatedrawingaxis", reinterpret_cast<void*>(cfml::cf_imagetranslatedrawingaxis));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagewrite", reinterpret_cast<void*>(cfml::cf_imagewrite));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagewritebase64", reinterpret_cast<void*>(cfml::cf_imagewritebase64));
    llvm::sys::DynamicLibrary::AddSymbol("cf_imagexordrawingmode", reinterpret_cast<void*>(cfml::cf_imagexordrawingmode));
    llvm::sys::DynamicLibrary::AddSymbol("cf_cfimage", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, void*, void*)>(cfml::cf_cfimage)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_initsamlauthrequest", reinterpret_cast<void*>(cfml::cf_initsamlauthrequest));
    llvm::sys::DynamicLibrary::AddSymbol("cf_initsamllogoutrequest", reinterpret_cast<void*>(cfml::cf_initsamllogoutrequest));
    llvm::sys::DynamicLibrary::AddSymbol("cf_inputbasen", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_inputbasen)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_insert", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*, const cfvariant*)>(cfml::cf_insert)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_interruptthread", reinterpret_cast<void*>(cfml::cf_interruptthread));
    llvm::sys::DynamicLibrary::AddSymbol("cf_invalidateoauthaccesstoken", reinterpret_cast<void*>(cfml::cf_invalidateoauthaccesstoken));
    llvm::sys::DynamicLibrary::AddSymbol("cf_invoke", reinterpret_cast<void*>(cfml::cf_invoke));
    llvm::sys::DynamicLibrary::AddSymbol("cf_invokecfclientfunction", reinterpret_cast<void*>(cfml::cf_invokecfclientfunction));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isauthenticated", reinterpret_cast<void*>(cfml::cf_isauthenticated));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isauthorized", reinterpret_cast<void*>(cfml::cf_isauthorized));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isbinary", reinterpret_cast<void*>(cfml::cf_isbinary));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isboolean", reinterpret_cast<void*>(cfml::cf_isboolean));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isclosure", reinterpret_cast<void*>(cfml::cf_isclosure));
    llvm::sys::DynamicLibrary::AddSymbol("cf_iscustomfunction", reinterpret_cast<void*>(cfml::cf_iscustomfunction));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isdateobject", reinterpret_cast<void*>(cfml::cf_isdateobject));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isddx", reinterpret_cast<void*>(cfml::cf_isddx));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isdebugmode", reinterpret_cast<void*>(cfml::cf_isdebugmode));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isdefined", reinterpret_cast<void*>(cfml::cf_isdefined));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isfileobject", reinterpret_cast<void*>(cfml::cf_isfileobject));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isimage", reinterpret_cast<void*>(cfml::cf_isimage));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isimagefile", reinterpret_cast<void*>(cfml::cf_isimagefile));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isinstanceof", reinterpret_cast<void*>(cfml::cf_isinstanceof));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isipv6", reinterpret_cast<void*>(cfml::cf_isipv6));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isjson", reinterpret_cast<void*>(cfml::cf_isjson));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isk2serverabroker", reinterpret_cast<void*>(cfml::cf_isk2serverabroker));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isk2serverdoccountexceeded", reinterpret_cast<void*>(cfml::cf_isk2serverdoccountexceeded));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isk2serveronline", reinterpret_cast<void*>(cfml::cf_isk2serveronline));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isleapyear", reinterpret_cast<void*>(cfml::cf_isleapyear));
    llvm::sys::DynamicLibrary::AddSymbol("cf_islocalhost", reinterpret_cast<void*>(cfml::cf_islocalhost));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isnull", reinterpret_cast<void*>(cfml::cf_isnull));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isnumeric", reinterpret_cast<void*>(cfml::cf_isnumeric));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isnumericdate", reinterpret_cast<void*>(cfml::cf_isnumericdate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isobject", reinterpret_cast<void*>(cfml::cf_isobject));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isonline", reinterpret_cast<void*>(cfml::cf_isonline));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ispdfarchive", reinterpret_cast<void*>(cfml::cf_ispdfarchive));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ispdffile", reinterpret_cast<void*>(cfml::cf_ispdffile));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ispdfobject", reinterpret_cast<void*>(cfml::cf_ispdfobject));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isprotected", reinterpret_cast<void*>(cfml::cf_isprotected));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isquery", reinterpret_cast<void*>(cfml::cf_isquery));
    llvm::sys::DynamicLibrary::AddSymbol("cf_issafehtml", reinterpret_cast<void*>(cfml::cf_issafehtml));
    llvm::sys::DynamicLibrary::AddSymbol("cf_issamllogoutresponse", reinterpret_cast<void*>(cfml::cf_issamllogoutresponse));
    llvm::sys::DynamicLibrary::AddSymbol("cf_issimplevalue", reinterpret_cast<void*>(cfml::cf_issimplevalue));
    llvm::sys::DynamicLibrary::AddSymbol("cf_issoaprequest", reinterpret_cast<void*>(cfml::cf_issoaprequest));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isspreadsheetfile", reinterpret_cast<void*>(cfml::cf_isspreadsheetfile));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isspreadsheetobject", reinterpret_cast<void*>(cfml::cf_isspreadsheetobject));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isstruct", reinterpret_cast<void*>(cfml::cf_isstruct));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isthreadinterrupted", reinterpret_cast<void*>(cfml::cf_isthreadinterrupted));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isuserinanyrole", reinterpret_cast<void*>(reinterpret_cast<cfvariant* (*)(const cfvariant*)>(cfml::cf_isuserinanyrole)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isuserinrole", reinterpret_cast<void*>(reinterpret_cast<cfvariant* (*)(const cfvariant*)>(cfml::cf_isuserinrole)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isuserloggedin", reinterpret_cast<void*>(cfml::cf_isuserloggedin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isvalid", reinterpret_cast<void*>(cfml::cf_isvalid));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isvalidoauthaccesstoken", reinterpret_cast<void*>(cfml::cf_isvalidoauthaccesstoken));
    llvm::sys::DynamicLibrary::AddSymbol("cf_iswddx", reinterpret_cast<void*>(cfml::cf_iswddx));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isxml", reinterpret_cast<void*>(cfml::cf_isxml));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isxmlattribute", reinterpret_cast<void*>(cfml::cf_isxmlattribute));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isxmldoc", reinterpret_cast<void*>(cfml::cf_isxmldoc));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isxmlelem", reinterpret_cast<void*>(cfml::cf_isxmlelem));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isxmlnode", reinterpret_cast<void*>(cfml::cf_isxmlnode));
    llvm::sys::DynamicLibrary::AddSymbol("cf_isxmlroot", reinterpret_cast<void*>(cfml::cf_isxmlroot));
    llvm::sys::DynamicLibrary::AddSymbol("cf_javacast", reinterpret_cast<void*>(cfml::cf_javacast));
    llvm::sys::DynamicLibrary::AddSymbol("cf_jsstringformat", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*)>(cfml::cf_jsstringformat)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_listeach", reinterpret_cast<void*>(cfml::cf_listeach));
    llvm::sys::DynamicLibrary::AddSymbol("cf_listfilter", reinterpret_cast<void*>(cfml::cf_listfilter));
    llvm::sys::DynamicLibrary::AddSymbol("cf_listgetduplicates", reinterpret_cast<void*>(cfml::cf_listgetduplicates));
    llvm::sys::DynamicLibrary::AddSymbol("cf_listmap", reinterpret_cast<void*>(cfml::cf_listmap));
    llvm::sys::DynamicLibrary::AddSymbol("cf_listqualify", reinterpret_cast<void*>(cfml::cf_listqualify));
    llvm::sys::DynamicLibrary::AddSymbol("cf_listreduce", reinterpret_cast<void*>(cfml::cf_listreduce));
    llvm::sys::DynamicLibrary::AddSymbol("cf_listremoveduplicates", reinterpret_cast<void*>(cfml::cf_listremoveduplicates));
    llvm::sys::DynamicLibrary::AddSymbol("cf_listsort", reinterpret_cast<void*>(cfml::cf_listsort));
    llvm::sys::DynamicLibrary::AddSymbol("cf_listvaluecount", reinterpret_cast<void*>(cfml::cf_listvaluecount));
    llvm::sys::DynamicLibrary::AddSymbol("cf_listvaluecountnocase", reinterpret_cast<void*>(cfml::cf_listvaluecountnocase));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ljustify", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_ljustify)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_location", reinterpret_cast<void*>(cfml::cf_location));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lscurrencyformat", reinterpret_cast<void*>(cfml::cf_lscurrencyformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lsdateformat", reinterpret_cast<void*>(cfml::cf_lsdateformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lsdatetimeformat", reinterpret_cast<void*>(cfml::cf_lsdatetimeformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lseurocurrencyformat", reinterpret_cast<void*>(cfml::cf_lseurocurrencyformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lsiscurrency", reinterpret_cast<void*>(cfml::cf_lsiscurrency));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lsisdate", reinterpret_cast<void*>(cfml::cf_lsisdate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lsisnumeric", reinterpret_cast<void*>(cfml::cf_lsisnumeric));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lsnumberformat", reinterpret_cast<void*>(cfml::cf_lsnumberformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lsparsecurrency", reinterpret_cast<void*>(cfml::cf_lsparsecurrency));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lsparsedatetime", reinterpret_cast<void*>(cfml::cf_lsparsedatetime));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lsparseeurocurrency", reinterpret_cast<void*>(cfml::cf_lsparseeurocurrency));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lsparsenumber", reinterpret_cast<void*>(cfml::cf_lsparsenumber));
    llvm::sys::DynamicLibrary::AddSymbol("cf_lstimeformat", reinterpret_cast<void*>(cfml::cf_lstimeformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_monthasstring", reinterpret_cast<void*>(cfml::cf_monthasstring));
    llvm::sys::DynamicLibrary::AddSymbol("cf_numberformat", reinterpret_cast<void*>(cfml::cf_numberformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_objectequals", reinterpret_cast<void*>(cfml::cf_objectequals));
    llvm::sys::DynamicLibrary::AddSymbol("cf_objectload", reinterpret_cast<void*>(cfml::cf_objectload));
    llvm::sys::DynamicLibrary::AddSymbol("cf_objectsave", reinterpret_cast<void*>(cfml::cf_objectsave));
    llvm::sys::DynamicLibrary::AddSymbol("cf_onwsauthenticate", reinterpret_cast<void*>(cfml::cf_onwsauthenticate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormclearsession", reinterpret_cast<void*>(cfml::cf_ormclearsession));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormcloseallsessions", reinterpret_cast<void*>(cfml::cf_ormcloseallsessions));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormclosesession", reinterpret_cast<void*>(cfml::cf_ormclosesession));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormevictcollection", reinterpret_cast<void*>(cfml::cf_ormevictcollection));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormevictentity", reinterpret_cast<void*>(cfml::cf_ormevictentity));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormevictqueries", reinterpret_cast<void*>(cfml::cf_ormevictqueries));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormexecutequery", reinterpret_cast<void*>(cfml::cf_ormexecutequery));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormflush", reinterpret_cast<void*>(cfml::cf_ormflush));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormflushall", reinterpret_cast<void*>(cfml::cf_ormflushall));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormgetsession", reinterpret_cast<void*>(cfml::cf_ormgetsession));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormgetsessionfactory", reinterpret_cast<void*>(cfml::cf_ormgetsessionfactory));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormindex", reinterpret_cast<void*>(cfml::cf_ormindex));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormindexpurge", reinterpret_cast<void*>(cfml::cf_ormindexpurge));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormreload", reinterpret_cast<void*>(cfml::cf_ormreload));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormsearch", reinterpret_cast<void*>(cfml::cf_ormsearch));
    llvm::sys::DynamicLibrary::AddSymbol("cf_ormsearchoffline", reinterpret_cast<void*>(cfml::cf_ormsearchoffline));
    llvm::sys::DynamicLibrary::AddSymbol("cf_parsedatetime", reinterpret_cast<void*>(cfml::cf_parsedatetime));
    llvm::sys::DynamicLibrary::AddSymbol("cf_precisionevaluate", reinterpret_cast<void*>(cfml::cf_precisionevaluate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_preservesinglequotes", reinterpret_cast<void*>(cfml::cf_preservesinglequotes));
    llvm::sys::DynamicLibrary::AddSymbol("cf_processsamllogoutrequest", reinterpret_cast<void*>(cfml::cf_processsamllogoutrequest));
    llvm::sys::DynamicLibrary::AddSymbol("cf_processsamlresponse", reinterpret_cast<void*>(cfml::cf_processsamlresponse));
    llvm::sys::DynamicLibrary::AddSymbol("cf_quarter", reinterpret_cast<void*>(cfml::cf_quarter));
    llvm::sys::DynamicLibrary::AddSymbol("cf_queryaddcolumn", reinterpret_cast<void*>(cfml::cf_queryaddcolumn));
    llvm::sys::DynamicLibrary::AddSymbol("cf_queryaddrow", reinterpret_cast<void*>(cfml::cf_queryaddrow));
    llvm::sys::DynamicLibrary::AddSymbol("cf_queryconvertforgrid", reinterpret_cast<void*>(cfml::cf_queryconvertforgrid));
    llvm::sys::DynamicLibrary::AddSymbol("cf_queryeach", reinterpret_cast<void*>(cfml::cf_queryeach));
    llvm::sys::DynamicLibrary::AddSymbol("cf_queryexecute", reinterpret_cast<void*>(cfml::cf_queryexecute));
    llvm::sys::DynamicLibrary::AddSymbol("cf_queryfilter", reinterpret_cast<void*>(cfml::cf_queryfilter));
    llvm::sys::DynamicLibrary::AddSymbol("cf_querygetresult", reinterpret_cast<void*>(cfml::cf_querygetresult));
    llvm::sys::DynamicLibrary::AddSymbol("cf_querygetrow", reinterpret_cast<void*>(cfml::cf_querygetrow));
    llvm::sys::DynamicLibrary::AddSymbol("cf_querykeyexists", reinterpret_cast<void*>(cfml::cf_querykeyexists));
    llvm::sys::DynamicLibrary::AddSymbol("cf_querymap", reinterpret_cast<void*>(cfml::cf_querymap));
    llvm::sys::DynamicLibrary::AddSymbol("cf_querynew", reinterpret_cast<void*>(cfml::cf_querynew));
    llvm::sys::DynamicLibrary::AddSymbol("cf_queryreduce", reinterpret_cast<void*>(cfml::cf_queryreduce));
    llvm::sys::DynamicLibrary::AddSymbol("cf_querysetcell", reinterpret_cast<void*>(cfml::cf_querysetcell));
    llvm::sys::DynamicLibrary::AddSymbol("cf_quotedvaluelist", reinterpret_cast<void*>(cfml::cf_quotedvaluelist));
    llvm::sys::DynamicLibrary::AddSymbol("cf_randrange", reinterpret_cast<void*>(cfml::cf_randrange));
    llvm::sys::DynamicLibrary::AddSymbol("cf_reescape", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*)>(cfml::cf_reescape)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_refind", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*)>(cfml::cf_refind)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_refindnocase", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*)>(cfml::cf_refindnocase)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_releasecomobject", reinterpret_cast<void*>(cfml::cf_releasecomobject));
    llvm::sys::DynamicLibrary::AddSymbol("cf_rematch", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_rematch)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_rematchnocase", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_rematchnocase)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_removecachedquery", reinterpret_cast<void*>(cfml::cf_removecachedquery));
    llvm::sys::DynamicLibrary::AddSymbol("cf_removechars", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*, const cfvariant*)>(cfml::cf_removechars)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_replacelist", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*)>(cfml::cf_replacelist)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_rereplace", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*)>(cfml::cf_rereplace)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_rereplacenocase", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*, const cfvariant*, const cfvariant*)>(cfml::cf_rereplacenocase)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_restdeleteapplication", reinterpret_cast<void*>(cfml::cf_restdeleteapplication));
    llvm::sys::DynamicLibrary::AddSymbol("cf_restinitapplication", reinterpret_cast<void*>(cfml::cf_restinitapplication));
    llvm::sys::DynamicLibrary::AddSymbol("cf_restsetresponse", reinterpret_cast<void*>(cfml::cf_restsetresponse));
    llvm::sys::DynamicLibrary::AddSymbol("cf_rjustify", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_rjustify)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_sendgatewaymessage", reinterpret_cast<void*>(cfml::cf_sendgatewaymessage));
    llvm::sys::DynamicLibrary::AddSymbol("cf_sendsamllogoutresponse", reinterpret_cast<void*>(cfml::cf_sendsamllogoutresponse));
    llvm::sys::DynamicLibrary::AddSymbol("cf_serialize", reinterpret_cast<void*>(cfml::cf_serialize));
    llvm::sys::DynamicLibrary::AddSymbol("cf_serializejson", reinterpret_cast<void*>(cfml::cf_serializejson));
    llvm::sys::DynamicLibrary::AddSymbol("cf_serializexml", reinterpret_cast<void*>(cfml::cf_serializexml));
    llvm::sys::DynamicLibrary::AddSymbol("cf_sessiongetmetadata", reinterpret_cast<void*>(cfml::cf_sessiongetmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_sessioninvalidate", reinterpret_cast<void*>(cfml::cf_sessioninvalidate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_sessionrotate", reinterpret_cast<void*>(cfml::cf_sessionrotate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setday", reinterpret_cast<void*>(cfml::cf_setday));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setencoding", reinterpret_cast<void*>(cfml::cf_setencoding));
    llvm::sys::DynamicLibrary::AddSymbol("cf_sethour", reinterpret_cast<void*>(cfml::cf_sethour));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setlocale", reinterpret_cast<void*>(cfml::cf_setlocale));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setminute", reinterpret_cast<void*>(cfml::cf_setminute));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setmonth", reinterpret_cast<void*>(cfml::cf_setmonth));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setprofilestring", reinterpret_cast<void*>(cfml::cf_setprofilestring));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setpropertystring", reinterpret_cast<void*>(cfml::cf_setpropertystring));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setsecond", reinterpret_cast<void*>(cfml::cf_setsecond));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setvariable", reinterpret_cast<void*>(cfml::cf_setvariable));
    llvm::sys::DynamicLibrary::AddSymbol("cf_setyear", reinterpret_cast<void*>(cfml::cf_setyear));
    llvm::sys::DynamicLibrary::AddSymbol("cf_sleep", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*)>(cfml::cf_sleep)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spanexcluding", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_spanexcluding)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spanincluding", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*)>(cfml::cf_spanincluding)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetaddautofilter", reinterpret_cast<void*>(cfml::cf_spreadsheetaddautofilter));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetaddcolumn", reinterpret_cast<void*>(cfml::cf_spreadsheetaddcolumn));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetaddfreezepane", reinterpret_cast<void*>(cfml::cf_spreadsheetaddfreezepane));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetaddimage", reinterpret_cast<void*>(cfml::cf_spreadsheetaddimage));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetaddinfo", reinterpret_cast<void*>(cfml::cf_spreadsheetaddinfo));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetaddpagebreaks", reinterpret_cast<void*>(cfml::cf_spreadsheetaddpagebreaks));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetaddprintgridlines", reinterpret_cast<void*>(cfml::cf_spreadsheetaddprintgridlines));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetaddrow", reinterpret_cast<void*>(cfml::cf_spreadsheetaddrow));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetaddrows", reinterpret_cast<void*>(cfml::cf_spreadsheetaddrows));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetaddsplitpane", reinterpret_cast<void*>(cfml::cf_spreadsheetaddsplitpane));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetcreatesheet", reinterpret_cast<void*>(cfml::cf_spreadsheetcreatesheet));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetdeletecolumn", reinterpret_cast<void*>(cfml::cf_spreadsheetdeletecolumn));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetdeletecolumns", reinterpret_cast<void*>(cfml::cf_spreadsheetdeletecolumns));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetdeleterow", reinterpret_cast<void*>(cfml::cf_spreadsheetdeleterow));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetdeleterows", reinterpret_cast<void*>(cfml::cf_spreadsheetdeleterows));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetformatcell", reinterpret_cast<void*>(cfml::cf_spreadsheetformatcell));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetformatcellrange", reinterpret_cast<void*>(cfml::cf_spreadsheetformatcellrange));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetformatcolumn", reinterpret_cast<void*>(cfml::cf_spreadsheetformatcolumn));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetformatcolumns", reinterpret_cast<void*>(cfml::cf_spreadsheetformatcolumns));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetformatrow", reinterpret_cast<void*>(cfml::cf_spreadsheetformatrow));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetformatrows", reinterpret_cast<void*>(cfml::cf_spreadsheetformatrows));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetgetcellcomment", reinterpret_cast<void*>(cfml::cf_spreadsheetgetcellcomment));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetgetcellformula", reinterpret_cast<void*>(cfml::cf_spreadsheetgetcellformula));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetgetcellvalue", reinterpret_cast<void*>(cfml::cf_spreadsheetgetcellvalue));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetgetcolumncount", reinterpret_cast<void*>(cfml::cf_spreadsheetgetcolumncount));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetgetcolumnwidth", reinterpret_cast<void*>(cfml::cf_spreadsheetgetcolumnwidth));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetgetlastrownumber", reinterpret_cast<void*>(cfml::cf_spreadsheetgetlastrownumber));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetgetprintorientation", reinterpret_cast<void*>(cfml::cf_spreadsheetgetprintorientation));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetgroupcolumns", reinterpret_cast<void*>(cfml::cf_spreadsheetgroupcolumns));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetgrouprows", reinterpret_cast<void*>(cfml::cf_spreadsheetgrouprows));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetinfo", reinterpret_cast<void*>(cfml::cf_spreadsheetinfo));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetisbinaryformat", reinterpret_cast<void*>(cfml::cf_spreadsheetisbinaryformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetiscolumnhidden", reinterpret_cast<void*>(cfml::cf_spreadsheetiscolumnhidden));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetisrowhidden", reinterpret_cast<void*>(cfml::cf_spreadsheetisrowhidden));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetisstreamingxmlformat", reinterpret_cast<void*>(cfml::cf_spreadsheetisstreamingxmlformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetisxmlformat", reinterpret_cast<void*>(cfml::cf_spreadsheetisxmlformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetmergecells", reinterpret_cast<void*>(cfml::cf_spreadsheetmergecells));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetnew", reinterpret_cast<void*>(cfml::cf_spreadsheetnew));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetread", reinterpret_cast<void*>(cfml::cf_spreadsheetread));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetreadbinary", reinterpret_cast<void*>(cfml::cf_spreadsheetreadbinary));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetremovecolumnbreak", reinterpret_cast<void*>(cfml::cf_spreadsheetremovecolumnbreak));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetremoveprintgridlines", reinterpret_cast<void*>(cfml::cf_spreadsheetremoveprintgridlines));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetremoverowbreak", reinterpret_cast<void*>(cfml::cf_spreadsheetremoverowbreak));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetremovesheet", reinterpret_cast<void*>(cfml::cf_spreadsheetremovesheet));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetremovesheetnumber", reinterpret_cast<void*>(cfml::cf_spreadsheetremovesheetnumber));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetrenamesheet", reinterpret_cast<void*>(cfml::cf_spreadsheetrenamesheet));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetactivesheet", reinterpret_cast<void*>(cfml::cf_spreadsheetsetactivesheet));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetactivesheetnumber", reinterpret_cast<void*>(cfml::cf_spreadsheetsetactivesheetnumber));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetcellcomment", reinterpret_cast<void*>(cfml::cf_spreadsheetsetcellcomment));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetcellformula", reinterpret_cast<void*>(cfml::cf_spreadsheetsetcellformula));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetcellvalue", reinterpret_cast<void*>(cfml::cf_spreadsheetsetcellvalue));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetcolumnbreak", reinterpret_cast<void*>(cfml::cf_spreadsheetsetcolumnbreak));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetcolumnhidden", reinterpret_cast<void*>(cfml::cf_spreadsheetsetcolumnhidden));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetcolumnwidth", reinterpret_cast<void*>(cfml::cf_spreadsheetsetcolumnwidth));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetfittopage", reinterpret_cast<void*>(cfml::cf_spreadsheetsetfittopage));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetfooter", reinterpret_cast<void*>(cfml::cf_spreadsheetsetfooter));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetfooterimage", reinterpret_cast<void*>(cfml::cf_spreadsheetsetfooterimage));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetheader", reinterpret_cast<void*>(cfml::cf_spreadsheetsetheader));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetheaderimage", reinterpret_cast<void*>(cfml::cf_spreadsheetsetheaderimage));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetrowbreak", reinterpret_cast<void*>(cfml::cf_spreadsheetsetrowbreak));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetrowheight", reinterpret_cast<void*>(cfml::cf_spreadsheetsetrowheight));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetsetrowhidden", reinterpret_cast<void*>(cfml::cf_spreadsheetsetrowhidden));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetshiftcolumns", reinterpret_cast<void*>(cfml::cf_spreadsheetshiftcolumns));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetshiftrows", reinterpret_cast<void*>(cfml::cf_spreadsheetshiftrows));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetungroupcolumns", reinterpret_cast<void*>(cfml::cf_spreadsheetungroupcolumns));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetungrouprows", reinterpret_cast<void*>(cfml::cf_spreadsheetungrouprows));
    llvm::sys::DynamicLibrary::AddSymbol("cf_spreadsheetwrite", reinterpret_cast<void*>(cfml::cf_spreadsheetwrite));
    llvm::sys::DynamicLibrary::AddSymbol("cf_storeaddacl", reinterpret_cast<void*>(cfml::cf_storeaddacl));
    llvm::sys::DynamicLibrary::AddSymbol("cf_storegetacl", reinterpret_cast<void*>(cfml::cf_storegetacl));
    llvm::sys::DynamicLibrary::AddSymbol("cf_storegetmetadata", reinterpret_cast<void*>(cfml::cf_storegetmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_storesetacl", reinterpret_cast<void*>(cfml::cf_storesetacl));
    llvm::sys::DynamicLibrary::AddSymbol("cf_storesetmetadata", reinterpret_cast<void*>(cfml::cf_storesetmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_streamingspreadsheetcleanup", reinterpret_cast<void*>(cfml::cf_streamingspreadsheetcleanup));
    llvm::sys::DynamicLibrary::AddSymbol("cf_streamingspreadsheetisstreamingxmlformat", reinterpret_cast<void*>(cfml::cf_streamingspreadsheetisstreamingxmlformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_streamingspreadsheetisxmlformat", reinterpret_cast<void*>(cfml::cf_streamingspreadsheetisxmlformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_streamingspreadsheetnew", reinterpret_cast<void*>(cfml::cf_streamingspreadsheetnew));
    llvm::sys::DynamicLibrary::AddSymbol("cf_streamingspreadsheetprocess", reinterpret_cast<void*>(cfml::cf_streamingspreadsheetprocess));
    llvm::sys::DynamicLibrary::AddSymbol("cf_streamingspreadsheetread", reinterpret_cast<void*>(cfml::cf_streamingspreadsheetread));
    llvm::sys::DynamicLibrary::AddSymbol("cf_stripcr", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*)>(cfml::cf_stripcr)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structappend", reinterpret_cast<void*>(cfml::cf_structappend));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structcopy", reinterpret_cast<void*>(cfml::cf_structcopy));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structeach", reinterpret_cast<void*>(cfml::cf_structeach));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structfilter", reinterpret_cast<void*>(cfml::cf_structfilter));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structfindkey", reinterpret_cast<void*>(cfml::cf_structfindkey));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structfindvalue", reinterpret_cast<void*>(cfml::cf_structfindvalue));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structget", reinterpret_cast<void*>(cfml::cf_structget));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structgetmetadata", reinterpret_cast<void*>(cfml::cf_structgetmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structmap", reinterpret_cast<void*>(cfml::cf_structmap));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structreduce", reinterpret_cast<void*>(cfml::cf_structreduce));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structsetmetadata", reinterpret_cast<void*>(cfml::cf_structsetmetadata));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structsort", reinterpret_cast<void*>(cfml::cf_structsort));
    llvm::sys::DynamicLibrary::AddSymbol("cf_structtosorted", reinterpret_cast<void*>(cfml::cf_structtosorted));
    llvm::sys::DynamicLibrary::AddSymbol("cf_threadjoin", reinterpret_cast<void*>(cfml::cf_threadjoin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_threadterminate", reinterpret_cast<void*>(cfml::cf_threadterminate));
    llvm::sys::DynamicLibrary::AddSymbol("cf_throw", reinterpret_cast<void*>(cfml::cf_throw));
    llvm::sys::DynamicLibrary::AddSymbol("cf_tobase64", reinterpret_cast<void*>(cfml::cf_tobase64));
    llvm::sys::DynamicLibrary::AddSymbol("cf_tobinary", reinterpret_cast<void*>(cfml::cf_tobinary));
    llvm::sys::DynamicLibrary::AddSymbol("cf_toscript", reinterpret_cast<void*>(cfml::cf_toscript));
    llvm::sys::DynamicLibrary::AddSymbol("cf_tostring", reinterpret_cast<void*>(cfml::cf_tostring));
    llvm::sys::DynamicLibrary::AddSymbol("cf_trace", reinterpret_cast<void*>(cfml::cf_trace));
    llvm::sys::DynamicLibrary::AddSymbol("cf_timer_begin", reinterpret_cast<void*>(cfml::cf_timer_begin));
    llvm::sys::DynamicLibrary::AddSymbol("cf_timer_end", reinterpret_cast<void*>(cfml::cf_timer_end));
    llvm::sys::DynamicLibrary::AddSymbol("cf_transactioncommit", reinterpret_cast<void*>(cfml::cf_transactioncommit));
    llvm::sys::DynamicLibrary::AddSymbol("cf_transactionrollback", reinterpret_cast<void*>(cfml::cf_transactionrollback));
    llvm::sys::DynamicLibrary::AddSymbol("cf_transactionsetsavepoint", reinterpret_cast<void*>(cfml::cf_transactionsetsavepoint));
    llvm::sys::DynamicLibrary::AddSymbol("cf_urldecode", reinterpret_cast<void*>(cfml::cf_urldecode));
    llvm::sys::DynamicLibrary::AddSymbol("cf_urlencodedformat", reinterpret_cast<void*>(cfml::cf_urlencodedformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_urlsessionformat", reinterpret_cast<void*>(cfml::cf_urlsessionformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_val", reinterpret_cast<void*>(cfml::cf_val));
    llvm::sys::DynamicLibrary::AddSymbol("cf_valuelist", reinterpret_cast<void*>(cfml::cf_valuelist));
    llvm::sys::DynamicLibrary::AddSymbol("cf_verifyclient", reinterpret_cast<void*>(cfml::cf_verifyclient));
    llvm::sys::DynamicLibrary::AddSymbol("cf_week", reinterpret_cast<void*>(cfml::cf_week));
    llvm::sys::DynamicLibrary::AddSymbol("cf_wrap", reinterpret_cast<void*>(static_cast<cfvariant*(*)(const cfvariant*, const cfvariant*, const cfvariant*)>(cfml::cf_wrap)));
    llvm::sys::DynamicLibrary::AddSymbol("cf_writedump", reinterpret_cast<void*>(cfml::cf_writedump));
    llvm::sys::DynamicLibrary::AddSymbol("cf_emit_writedump", reinterpret_cast<void*>(cfml::cf_emit_writedump));
    llvm::sys::DynamicLibrary::AddSymbol("cf_writelog", reinterpret_cast<void*>(cfml::cf_writelog));
    llvm::sys::DynamicLibrary::AddSymbol("cf_trace_tag", reinterpret_cast<void*>(cfml::cf_trace_tag));
    llvm::sys::DynamicLibrary::AddSymbol("cf_wsgetallchannels", reinterpret_cast<void*>(cfml::cf_wsgetallchannels));
    llvm::sys::DynamicLibrary::AddSymbol("cf_wsgetsubscribers", reinterpret_cast<void*>(cfml::cf_wsgetsubscribers));
    llvm::sys::DynamicLibrary::AddSymbol("cf_wspublish", reinterpret_cast<void*>(cfml::cf_wspublish));
    llvm::sys::DynamicLibrary::AddSymbol("cf_wssendmessage", reinterpret_cast<void*>(cfml::cf_wssendmessage));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xmlchildpos", reinterpret_cast<void*>(cfml::cf_xmlchildpos));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xmlelemnew", reinterpret_cast<void*>(cfml::cf_xmlelemnew));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xmlformat", reinterpret_cast<void*>(cfml::cf_xmlformat));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xmlgetnodetype", reinterpret_cast<void*>(cfml::cf_xmlgetnodetype));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xmlnew", reinterpret_cast<void*>(cfml::cf_xmlnew));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xmlparse", reinterpret_cast<void*>(cfml::cf_xmlparse));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xmlsearch", reinterpret_cast<void*>(cfml::cf_xmlsearch));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xmltransform", reinterpret_cast<void*>(cfml::cf_xmltransform));
    llvm::sys::DynamicLibrary::AddSymbol("cf_xmlvalidate", reinterpret_cast<void*>(cfml::cf_xmlvalidate));
}

template_fn llvm_codegen::compile(const string &pathname, bool print_ast)
{
    // Read + decode the source following ColdFusion's input-encoding order
    // (BOM -> pageEncoding -> ICU detection -> default charset), producing the
    // internal UTF-8 representation that the textparser expects.
    TemplateReadResult src = readTemplateFile(std::string(pathname.constData(), pathname.length()));
    parser parse;
    static const char emptyStr = '\0';
    const char *text = src.utf8Text.empty() ? &emptyStr : src.utf8Text.data();
    parse.parse(text, static_cast<int>(src.utf8Text.size()), TEXTPARSER_ENCODING_UTF_8,
                pathname.constData());
    auto t0 = std::chrono::steady_clock::now();
    template_fn result = compile_parsed(parse, pathname.constData(), print_ast);
    auto t1 = std::chrono::steady_clock::now();
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    // stderr: stdout carries the template's response payload (verify_with_coldfusion.py
    // compares it byte-for-byte against Adobe CF).
    fprintf(stderr, "[WebStrada] compiled %s (%lldms)\n", pathname.constData(), ms);
    return result;
}

template_fn llvm_codegen::compile_string(const char *buffer, size_t size, bool print_ast, const char *name)
{
    std::vector<char> bytes(buffer, buffer + size);
    TemplateReadResult src = readTemplateBuffer(bytes);
    parser parse;
    static const char emptyStr = '\0';
    const char *text = src.utf8Text.empty() ? &emptyStr : src.utf8Text.data();
    parse.parse(text, static_cast<int>(src.utf8Text.size()), TEXTPARSER_ENCODING_UTF_8);
    return compile_parsed(parse, name, print_ast);
}

template_fn llvm_codegen::compile_parsed(parser &parse, const char *name, bool print_ast)
{
    // Reset the compile-time variable-binding slot registry for this template.
    // The slots are per-module allocas in this template's main function, so
    // state must not leak from a previous template compiled on this thread.
    g_varFastSlots.clear();
    g_importPrefixes.clear();

    // Resolve line numbers against this parser's line map for the whole compile.
    TextparserHandleGuard handleGuard(parse.handle());

    // Generate IR
    auto module = new llvm::Module(name ? name : "template", m_context);
    llvm::IRBuilder<> builder(m_context);

    auto mainfunc = llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "main", module);

    auto *mainEntry = llvm::BasicBlock::Create(m_context, "entry", mainfunc);
    builder.SetInsertPoint(mainEntry);

    auto *mainCleanupBB = llvm::BasicBlock::Create(m_context, "main.cleanup", mainfunc);
    ScopedCodegenState<llvm::BasicBlock*> cleanupBBGuard(g_currentFuncCleanupBB, mainCleanupBB);

    auto *fCleanupSave = getOrCreateHelper(module, builder, "cfvariant_cleanup_save", builder.getInt64Ty(), {});
    llvm::Value *mainSavepoint = builder.CreateCall(fCleanupSave, {});

    // Push this template's call-stack frame (full pathname, line 0) as the very
    // first thing executed; popped on every exit path below.
    emitStackPush(module, builder);

    auto out = mainfunc->getArg(0);
    auto cgi = mainfunc->getArg(1);
    auto server = mainfunc->getArg(2);
    auto cookie = mainfunc->getArg(3);
    auto application = mainfunc->getArg(4);
    auto session = mainfunc->getArg(5);
    auto url = mainfunc->getArg(6);
    auto form = mainfunc->getArg(7);
    auto variables = mainfunc->getArg(8);

    const char *cfm_text = parse.get_text();
    size_t cfm_text_size = parse.get_text_size();

    // Convert parser flat tokens into converted structure
    std::vector<TextParserTokenItem> topLevelTokens;
    while (auto token = parse.next_token()) {
        if (token->token_id == TextParser_cfml_Operator && token->child) {
            auto opChild = token->child;
            while (opChild) {
                if (opChild->token_id >= TextParser_cfml_ScriptTagPair &&
                    opChild->token_id <= TextParser_cfml_ArrayIndex) {
                    topLevelTokens.push_back(convertToken(opChild));
                }
                opChild = opChild->next;
            }
        } else {
            topLevelTokens.push_back(convertToken(token));
        }
    }

    // ---- User-defined functions (hoisted across the whole template in CF) ----
    // Collect every page-level `function name(...) { ... }` declaration, enforce
    // CF's compile-time name rules, compile each body into a JIT function and
    // register it into the variables scope at template entry so UDFs are
    // callable before their textual definition.
    std::vector<UdfDef> pageUdfs;
    // A page-level <cfscript> may sit directly at top level or nested inside
    // <cfoutput> (an OutputTagPair → OutputExpression wraps the ScriptTagPair);
    // scan the whole token tree recursively so those UDFs are hoisted too.
    std::function<void(const std::vector<TextParserTokenItem>&)> scanScripts;
    scanScripts = [&](const std::vector<TextParserTokenItem> &tokens) {
        for (const auto &t : tokens) {
            if (t.token_id == TextParser_cfml_ScriptTagPair) {
                for (const auto &child : t.children) {
                    if (child.token_id == TextParser_cfml_ScriptExpression) {
                        collectFunctionDecls(child.children, cfm_text, pageUdfs);
                    }
                }
            } else if (t.token_id == TextParser_cfml_OutputTagPair ||
                       t.token_id == TextParser_cfml_OutputExpression ||
                       t.token_id == TextParser_cfml_CodeBlock) {
                scanScripts(t.children);
            }
        }
    };
    scanScripts(topLevelTokens);
    // Tag-form `<cffunction>` declarations. Their bodies are sibling tokens in
    // the top-level stream, so a forward scan collects each block (even inside
    // control flow, which CF hoists to the page scope too). A tag-form and a
    // script-form function with the same name collide below ("declared twice").
    collectTagFunctionDecls(topLevelTokens, cfm_text, pageUdfs);
    {
        std::set<std::string> seenNames;
        for (auto &def : pageUdfs) {
            if (def.name.empty()) continue;
            std::string upper = def.name;
            for (auto &c : upper) c = (char)toupper((unsigned char)c);
            bool valid = !def.name.empty();
            for (size_t ci = 0; valid && ci < def.name.size(); ci++) {
                char ch = def.name[ci];
                valid = (ci == 0) ? (isalpha((unsigned char)ch) || ch == '_')
                                  : (isalnum((unsigned char)ch) || ch == '_');
            }
            if (!valid) {
                std::string detail = "The name " + def.name + " contains illegal characters.";
                throw webstrada::exception("Invalid name for user-defined function.", string(detail.c_str()));
            }
            if (cfml::cf_is_known_function_name(upper.c_str())) {
                std::string detail = "The name " + def.name + " is the name of a built-in ColdFusion function.";
                throw webstrada::exception("The names of user-defined functions cannot be the same as built-in ColdFusion functions.", string(detail.c_str()));
            }
            if (seenNames.count(upper)) {
                std::string detail = "The routine " + def.name + " has been declared twice in the same file.";
                throw webstrada::exception("Routines cannot be declared more than once.", string(detail.c_str()));
            }
            seenNames.insert(upper);
        }
        for (const auto &def : pageUdfs) {
            if (def.name.empty()) continue;
            compileUdfFunction(module, m_context, builder, "udf_" + def.name, def, cfm_text, cfm_text_size);
        }
    }
    // Register page-level UDFs into the variables scope at template entry.
    builder.SetInsertPoint(mainEntry);
    {
        auto *fCreateUdf = getOrCreateHelper(module, builder, "cfvariant_create_udf", builder.getPtrTy(),
                                             {builder.getPtrTy(), builder.getPtrTy(), builder.getInt1Ty(), builder.getPtrTy(), builder.getPtrTy()});
        auto *fAssign = getOrCreateHelper(module, builder, "cfvariant_assign", builder.getPtrTy(),
                                          {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                           builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                           builder.getPtrTy(), builder.getPtrTy()});
        for (const auto &def : pageUdfs) {
            if (def.name.empty()) continue;
            std::string upper = def.name;
            for (auto &c : upper) c = (char)toupper((unsigned char)c);
            llvm::Function *udfFn = module->getFunction("udf_" + def.name);
            if (!udfFn) continue;
            std::string metaBlob = buildUdfMetaBlob(def, cfm_text);
            llvm::Value *udfVal = emitCall(builder, fCreateUdf, {
                builder.CreateGlobalString(llvm::StringRef(def.name), "", 0, module, true),
                udfFn, builder.getInt1(false), variables,
                builder.CreateGlobalString(llvm::StringRef(metaBlob.data(), metaBlob.size()), "", 0, module, true)});
            emitCall(builder, fAssign, {cgi, server, cookie, application, session, url, form, variables,
                                         builder.CreateGlobalString(llvm::StringRef(upper), "", 0, module, true), udfVal});
        }
    }

    size_t index = 0;
    size_t pos = 0;
    std::vector<LoopInfo> loopStack;

    // Read the server-wide config in the AST-constructing layer: when whitespace
    // management is enabled (ColdFusion's default), whitespace-only regions
    // between CFML tags are collapsed/removed the way ColdFusion does it.
    WsFlag wsFlag;
    WhitespaceState wsTop(config::enableWhitespaceManagement, wsFlag);

    compile_token_list(topLevelTokens, index, pos, m_context, module, builder, mainfunc,
                       out, wsTop, cgi, server, cookie, application, session, url, form, variables,
                       cfm_text, cfm_text_size, loopStack);

    if (pos < cfm_text_size) {
        wsTop.feed(module, builder, out, cfm_text + pos, cfm_text_size - pos, WsRight::DocumentEnd);
    }
    wsTop.finish(module, builder, out, WsRight::DocumentEnd);

    auto *fRestore = getOrCreateHelper(module, builder, "cfvariant_cleanup_restore",
                                       builder.getVoidTy(), {builder.getInt64Ty()});
    builder.CreateCall(fRestore, {mainSavepoint});
    emitStackPop(module, builder);
    builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0, false));

    // Exception unwinding landing pad for main template function
    builder.SetInsertPoint(mainCleanupBB);
    auto *lpTy = llvm::StructType::get(m_context, {builder.getPtrTy(), builder.getInt32Ty()});
    auto *lp = builder.CreateLandingPad(lpTy, 0, "main.lp");
    lp->setCleanup(true);
    // Snapshot the stack into the in-flight exception (the request-level error
    // page / cferror handler reads it), then pop this frame before resuming.
    llvm::Value *mainExn = builder.CreateExtractValue(lp, 0, "main.exn");
    emitStackCaptureOnException(module, builder, mainExn);
    emitStackPop(module, builder);
    builder.CreateCall(fRestore, {mainSavepoint});
    builder.CreateResume(lp);

    // Attach the C++ Itanium personality to every JIT function so `invoke`/
    // `landingpad` can participate in exception handling. Runtime exceptions
    // thrown by C++ helpers then unwind through ALL JIT frames (plain calls in
    // between are covered because each function carries a personality + .eh_frame
    // entry), and only stop at a function that actually has a landing pad.
    // Reuse the single module-wide personality (a try/catch compiled earlier may
    // already have created one); creating a duplicate would be auto-renamed to
    // `__gxx_personality_v0.1` and its .eh_frame references could not be resolved
    // by MCJIT, leaving external relocations unpatched (SIGSEGV).
    {
        llvm::Function *pers = getOrCreatePersonality(module, builder, nullptr);
        for (auto &f : module->functions()) {
            if (!f.hasPersonalityFn()) f.setPersonalityFn(pers);
        }
    }

    if (print_ast) {
        module->print(llvm::outs(), nullptr);
    }

    // Generating code.
    std::string error;
    std::unique_ptr<llvm::ExecutionEngine> engine(llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module))
                      .setErrorStr(&error)
                      //.setOptLevel(llvm::CodeGenOpt::Aggressive)
                      .setEngineKind(llvm::EngineKind::JIT)
                      .create());
    if (!engine.get()) {
        throw webstrada::exception("Code generation error!", "Failed to create EngineBuilder.");
    }

    engine.get()->finalizeObject();

    auto funcPtr = engine.get()->getPointerToNamedFunction("main");

    m_engines.push_back(std::move(engine));

    return reinterpret_cast<void (*)(string *out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables)>(funcPtr);
}

// Parse a script-form component attribute list (extends="..", persistent="..",
// ...) from a flat token stream: a sequence of Variable = quoted-value triples.
static void parseScriptComponentAttrs(const std::vector<TextParserTokenItem> &tokens,
                                      size_t begin, size_t end,
                                      const char *cfm_text,
                                      std::map<std::string, std::string> &attrs)
{
    size_t i = begin;
    while (i + 2 < end) {
        const auto &nameTok = tokens[i];
        if (nameTok.token_id != TextParser_cfml_Variable ||
            !isOperatorToken(tokens[i + 1].token_id)) {
            i++;
            continue;
        }
        const auto &valTok = tokens[i + 2];
        if (valTok.token_id != TextParser_cfml_DoubleString &&
            valTok.token_id != TextParser_cfml_SingleString) {
            i++;
            continue;
        }
        std::string name = tokenText(nameTok, cfm_text);
        std::string low = lowercase(name);
        std::string val = tokenText(valTok, cfm_text);
        if (val.size() >= 2) val = val.substr(1, val.size() - 2);
        attrs[low] = val;
        i += 3;
    }
}

// Returns the index of the `function` keyword if `tokens[start]` begins a named
// script-form function declaration (optionally prefixed by access/return-type/
// `final` modifier Variables), or size_t(-1) when it is not a declaration (a
// closure `function(...) {...}`, a constructor call, an assignment, etc.).
static size_t scriptFunctionKeywordIndex(const std::vector<TextParserTokenItem> &tokens,
                                         size_t start, const char *cfm_text)
{
    if (start >= tokens.size()) return size_t(-1);
    const auto &t = tokens[start];
    if (t.token_id == TextParser_cfml_Keyword && kwTextIs(t, cfm_text, "function")) {
        return (start + 1 < tokens.size() && tokens[start + 1].token_id == TextParser_cfml_Function)
                   ? start : size_t(-1);
    }
    if (t.token_id != TextParser_cfml_Variable) return size_t(-1);
    // Look ahead over consecutive modifier Variables (access, return type,
    // `final`) to a `function` keyword followed by a function name.
    size_t j = start;
    while (j < tokens.size() && tokens[j].token_id == TextParser_cfml_Variable) j++;
    if (j < tokens.size() && tokens[j].token_id == TextParser_cfml_Keyword &&
        kwTextIs(tokens[j], cfm_text, "function") &&
        j + 1 < tokens.size() && tokens[j + 1].token_id == TextParser_cfml_Function) {
        return j;
    }
    return size_t(-1);
}

// Parse a script-form function declaration starting at `start` (the modifier
// Variable or the `function` keyword). Returns the index past the CodeBlock.
static size_t parseScriptFunctionDecl(const std::vector<TextParserTokenItem> &tokens,
                                      size_t start, const char *cfm_text, UdfDef &def)
{
    size_t fnKeyword = scriptFunctionKeywordIndex(tokens, start, cfm_text);
    if (fnKeyword == size_t(-1)) {
        def = UdfDef();
        return start;
    }
    std::string access = "public";
    std::string returnTypeBeforeName;
    for (size_t i = start; i < fnKeyword; i++) {
        if (tokens[i].token_id == TextParser_cfml_Variable) {
            std::string w = tokenText(tokens[i], cfm_text);
            std::string low = lowercase(w);
            if (low == "public" || low == "private" || low == "package" || low == "remote") {
                access = low;
            } else if (low == "final") {
                // final method modifier — accepted, no special behavior
            } else if (returnTypeBeforeName.empty()) {
                returnTypeBeforeName = w;
            }
        }
    }
    size_t after = parseFunctionDecl(tokens, fnKeyword, cfm_text, def);
    def.access = access;
    if (!returnTypeBeforeName.empty() && def.returnType.empty()) {
        def.returnType = returnTypeBeforeName;
    }
    return after;
}

// Parse a script-form `property name=".." type=".." ...;` declaration.
static size_t parseScriptPropertyDecl(const std::vector<TextParserTokenItem> &tokens,
                                      size_t start, const char *cfm_text,
                                      ComponentProperty &prop)
{
    size_t i = start + 1; // skip the `property` Variable
    while (i + 2 < tokens.size() &&
           tokens[i].token_id == TextParser_cfml_Variable &&
           isOperatorToken(tokens[i + 1].token_id) &&
           (tokens[i + 2].token_id == TextParser_cfml_DoubleString ||
            tokens[i + 2].token_id == TextParser_cfml_SingleString ||
            tokens[i + 2].token_id == TextParser_cfml_Number ||
            tokens[i + 2].token_id == TextParser_cfml_Boolean)) {
        std::string attrName = tokenText(tokens[i], cfm_text);
        std::string low = lowercase(attrName);
        std::string val = tokenText(tokens[i + 2], cfm_text);
        if (tokens[i + 2].token_id == TextParser_cfml_DoubleString ||
            tokens[i + 2].token_id == TextParser_cfml_SingleString) {
            if (val.size() >= 2) val = val.substr(1, val.size() - 2);
        }
        if (low == "name") prop.name = val;
        else if (low == "type") prop.type = val;
        else if (low == "default") prop.defaultText = val;
        else if (low == "access") prop.access = val;
        i += 3;
    }
    while (i < tokens.size() && tokens[i].token_id != TextParser_cfml_ExpressionEnd) i++;
    if (i < tokens.size()) i++;
    return i;
}

// Parse a script-form CFC's top-level token (a single ScriptExpression whose
// children begin with a `component` Variable, possibly after `import`
// statements / comments). Returns true when the file is a script-form CFC; on
// success fills `attrs`, `methodDefs`, `props` and `construction` (the body
// statements minus method/property declarations).
static bool parseScriptFormComponent(
    const std::vector<TextParserTokenItem> &tokens,
    const char *cfm_text,
    std::map<std::string, std::string> &attrs,
    std::vector<UdfDef> &methodDefs,
    std::vector<ComponentProperty> &props,
    std::vector<TextParserTokenItem> &construction,
    bool *isInterface)
{
    if (tokens.empty() || tokens[0].token_id != TextParser_cfml_ScriptExpression) return false;
    const auto &top = tokens[0].children;

    // Locate the `component`/`interface` keyword Variable (skip leading
    // comments/imports).
    size_t compIdx = top.size();
    for (size_t i = 0; i < top.size(); i++) {
        if (top[i].token_id == TextParser_cfml_Variable &&
            (kwTextIs(top[i], cfm_text, "component") || kwTextIs(top[i], cfm_text, "interface"))) {
            compIdx = i;
            break;
        }
    }
    if (compIdx == top.size()) return false;
    if (isInterface) {
        *isInterface = kwTextIs(top[compIdx], cfm_text, "interface");
    }

    // The body CodeBlock follows the keyword and any attributes.
    size_t cbIdx = compIdx + 1;
    for (; cbIdx < top.size(); cbIdx++) {
        if (top[cbIdx].token_id == TextParser_cfml_CodeBlock) break;
    }
    if (cbIdx >= top.size()) {
        throw webstrada::exception("cfcomponent", "The component file does not contain a component body.");
    }

    // Component attributes between the keyword and the CodeBlock.
    parseScriptComponentAttrs(top, compIdx + 1, cbIdx, cfm_text, attrs);

    // The body is the CodeBlock's ScriptExpression children.
    const auto &cb = top[cbIdx];
    std::vector<TextParserTokenItem> body;
    if (!cb.children.empty() && cb.children[0].token_id == TextParser_cfml_ScriptExpression) {
        body = cb.children[0].children;
    } else {
        body = cb.children;
    }

    // Walk the body: method declarations, property declarations, and the
    // construction body (everything else).
    size_t i = 0;
    while (i < body.size()) {
        const auto &t = body[i];
        if (t.token_id == TextParser_cfml_Variable && kwTextIs(t, cfm_text, "property")) {
            ComponentProperty prop;
            i = parseScriptPropertyDecl(body, i, cfm_text, prop);
            if (!prop.name.empty()) props.push_back(prop);
            continue;
        }
        // A script-form processing directive `pageencoding "..";` is a
        // compile-time directive (the template_reader already applied it and
        // ran the BOM-conflict check), so the statement is skipped like CF
        // removes the PAGEENCODING attribute before code generation.
        if (t.token_id == TextParser_cfml_Variable && kwTextIs(t, cfm_text, "pageencoding") &&
            i + 1 < body.size() &&
            (body[i + 1].token_id == TextParser_cfml_SingleString ||
             body[i + 1].token_id == TextParser_cfml_DoubleString)) {
            i += 2;
            if (i < body.size() && body[i].token_id == TextParser_cfml_ExpressionEnd) i++;
            continue;
        }
        size_t fnKeyword = scriptFunctionKeywordIndex(body, i, cfm_text);
        if (fnKeyword != size_t(-1)) {
            UdfDef def;
            i = parseScriptFunctionDecl(body, i, cfm_text, def);
            methodDefs.push_back(std::move(def));
            continue;
        }
        construction.push_back(t);
        i++;
    }
    return true;
}

ComponentInfo *llvm_codegen::compileComponent(const string &pathname)
{
    TemplateReadResult src = readTemplateFile(std::string(pathname.constData(), pathname.length()));
    parser parse;
    static const char emptyStr = '\0';
    const char *text = src.utf8Text.empty() ? &emptyStr : src.utf8Text.data();
    parse.parse(text, static_cast<int>(src.utf8Text.size()), TEXTPARSER_ENCODING_UTF_8,
                pathname.constData());
    auto t0 = std::chrono::steady_clock::now();

    // Resolve line numbers against this parser's line map for the whole compile.
    TextparserHandleGuard handleGuard(parse.handle());

    g_varFastSlots.clear();

    auto module = new llvm::Module(pathname.constData(), m_context);
    llvm::IRBuilder<> builder(m_context);

    const char *cfm_text = parse.get_text();
    size_t cfm_text_size = parse.get_text_size();

    // Convert parser flat tokens into converted structure.
    std::vector<TextParserTokenItem> tokens;
    while (auto *token = parse.next_token()) {
        if (token->token_id == TextParser_cfml_Operator && token->child) {
            auto opChild = token->child;
            while (opChild) {
                if (opChild->token_id >= TextParser_cfml_ScriptTagPair &&
                    opChild->token_id <= TextParser_cfml_ArrayIndex) {
                    tokens.push_back(convertToken(opChild));
                }
                opChild = opChild->next;
            }
        } else {
            tokens.push_back(convertToken(token));
        }
    }

    std::map<std::string, std::string> attrs;
    std::vector<UdfDef> methodDefs;
    std::vector<ComponentProperty> scriptProps;
    std::vector<TextParserTokenItem> construction; // script-form construction body tokens
    bool scriptForm = false;
    bool scriptIsInterface = false;
    scriptForm = parseScriptFormComponent(tokens, cfm_text, attrs, methodDefs, scriptProps, construction, &scriptIsInterface);

    size_t compStart = tokens.size();
    size_t compEnd = tokens.size();
    bool isInterface = scriptForm ? scriptIsInterface : false;
    if (!scriptForm) {
        // Locate the top-level <cfcomponent> / <cfinterface> ... </...> (the
        // whole file, typically).
        const char *openTag = isInterface ? "cfinterface" : "cfcomponent";
        const char *closeTag = isInterface ? "cfinterface" : "cfcomponent";
        {
            int depth = 0;
            for (size_t i = 0; i < tokens.size(); i++) {
                const auto &tok = tokens[i];
                if (tok.token_id == TextParser_cfml_StartTag &&
                    tagNameOf(tok, cfm_text) == openTag) {
                    if (compStart == tokens.size()) compStart = i;
                    depth++;
                } else if (tok.token_id == TextParser_cfml_EndTag &&
                           tagNameOf(tok, cfm_text) == closeTag) {
                    depth--;
                    if (depth == 0) { compEnd = i; break; }
                }
            }
        }
        if (compStart == tokens.size() || compEnd == tokens.size()) {
            // No <cfcomponent>; check for a top-level <cfinterface>.
            isInterface = true;
            const char *openTag2 = "cfinterface";
            compStart = tokens.size();
            compEnd = tokens.size();
            {
                int depth = 0;
                for (size_t i = 0; i < tokens.size(); i++) {
                    const auto &tok = tokens[i];
                    if (tok.token_id == TextParser_cfml_StartTag &&
                        tagNameOf(tok, cfm_text) == openTag2) {
                        if (compStart == tokens.size()) compStart = i;
                        depth++;
                    } else if (tok.token_id == TextParser_cfml_EndTag &&
                               tagNameOf(tok, cfm_text) == openTag2) {
                        depth--;
                        if (depth == 0) { compEnd = i; break; }
                    }
                }
            }
            if (compStart == tokens.size() || compEnd == tokens.size()) {
                throw webstrada::exception("cfcomponent", "The component file does not contain a <cfcomponent> tag.");
            }
        }
        parseTagAttrs(tokens[compStart], cfm_text, attrs);
    }

    auto *info = new ComponentInfo();
    info->cfcPath = std::string(pathname.constData(), pathname.length());
    std::string base = info->cfcPath;
    size_t slash = base.find_last_of('/');
    if (slash != std::string::npos) base = base.substr(slash + 1);
    size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);
    info->name = base;
    info->path = base;
    info->isInterface = isInterface;
    if (isInterface) {
        auto dnIt = attrs.find("displayname");
        if (dnIt != attrs.end()) info->displayName = dnIt->second;
        auto hIt = attrs.find("hint");
        if (hIt != attrs.end()) info->hint = hIt->second;
        auto exIt = attrs.find("extends");
        if (exIt != attrs.end() && !exIt->second.empty()) {
            // Comma-delimited list of interfaces this interface extends.
            std::string cur;
            for (char c : exIt->second) {
                if (c == ',') {
                    std::string t = cur;
                    while (!t.empty() && (t.front() == ' ' || t.front() == '\t')) t.erase(t.begin());
                    while (!t.empty() && (t.back() == ' ' || t.back() == '\t')) t.pop_back();
                    if (!t.empty()) info->extendsList.push_back(t);
                    cur.clear();
                } else {
                    cur += c;
                }
            }
            std::string t = cur;
            while (!t.empty() && (t.front() == ' ' || t.front() == '\t')) t.erase(t.begin());
            while (!t.empty() && (t.back() == ' ' || t.back() == '\t')) t.pop_back();
            if (!t.empty()) info->extendsList.push_back(t);
        }
    } else {
        auto impIt = attrs.find("implements");
        if (impIt != attrs.end()) info->implementsText = impIt->second;
        auto extIt = attrs.find("extends");
        if (extIt != attrs.end()) info->extendsPath = extIt->second;
    }

    if (scriptForm) {
        info->properties = std::move(scriptProps);
    } else {
        // Scan the component body: <cfproperty> declarations, <cffunction> method
        // declarations (collected for compilation) and everything else (the
        // construction body, compiled below with the cffunction/cfproperty blocks
        // skipped by compile_token_list).
        for (size_t i = compStart + 1; i < compEnd; i++) {
            const auto &tok = tokens[i];
            if (tok.token_id == TextParser_cfml_StartTag) {
                std::string tn = tagNameOf(tok, cfm_text);
                if (tn == "cfproperty") {
                    std::map<std::string, std::string> pattrs;
                    parseTagAttrs(tok, cfm_text, pattrs);
                    ComponentProperty prop;
                    auto it = pattrs.find("name");
                    if (it != pattrs.end()) prop.name = it->second;
                    it = pattrs.find("type");
                    if (it != pattrs.end()) prop.type = it->second;
                    it = pattrs.find("default");
                    if (it != pattrs.end()) prop.defaultText = it->second;
                    it = pattrs.find("access");
                    if (it != pattrs.end()) prop.access = it->second;
                    if (!prop.name.empty()) info->properties.push_back(prop);
                } else if (tn == "cffunction") {
                    UdfDef def;
                    i = parseTagFunctionDecl(tokens, i, cfm_text, def);
                    methodDefs.push_back(std::move(def));
                    i = (i > 0) ? i - 1 : i;
                }
            }
        }
    }

    // Enforce CF's method-name rules and fill in the definition.
    {
        std::set<std::string> seenNames;
        for (auto &def : methodDefs) {
            std::string upper = def.name;
            for (auto &c : upper) c = (char)toupper((unsigned char)c);
            bool valid = !def.name.empty();
            for (size_t ci = 0; valid && ci < def.name.size(); ci++) {
                char ch = def.name[ci];
                valid = (ci == 0) ? (isalpha((unsigned char)ch) || ch == '_')
                                  : (isalnum((unsigned char)ch) || ch == '_');
            }
            if (!valid) {
                throw webstrada::exception("Invalid name for user-defined function.",
                    string(("The name " + def.name + " contains illegal characters.").c_str()));
            }
            if (seenNames.count(upper)) {
                throw webstrada::exception("Routines cannot be declared more than once.",
                    string(("The routine " + def.name + " has been declared twice in the same file.").c_str()));
            }
            seenNames.insert(upper);
            ComponentMethod m;
            m.name = upper;
            m.declaredName = def.name;
            std::string acc = def.access.empty() ? "public" : def.access;
            for (auto &c : acc) c = (char)tolower((unsigned char)c);
            m.access = acc;
            m.returnType = def.returnType;
            m.paramNames = def.paramNames;
            m.paramTypes = def.paramTypes;
            for (size_t pi = 0; pi < def.paramNames.size(); pi++) {
                UdfParamInfo pinfo;
                pinfo.name = webstrada::string(def.paramNames[pi].c_str());
                pinfo.type = webstrada::string(def.paramTypes[pi].c_str());
                if (pi < def.paramDefaultsRaw.size()) {
                    pinfo.defaultValue = webstrada::string(def.paramDefaultsRaw[pi].c_str());
                }
                m.params.push_back(pinfo);
                m.paramRequired.push_back(pi < def.paramRequired.size() && def.paramRequired[pi]);
            }
            info->methods.push_back(m);
        }
    }

    // ---- Compile the construction body (component_body_fn) ----
    // Signature: (out, cgi, server, cookie, application, session, url, form,
    // variablesScope, thisScope) -> void
    auto *bodyFn = llvm::Function::Create(
        llvm::FunctionType::get(builder.getVoidTy(),
            std::vector<llvm::Type*>(10, builder.getPtrTy()), false),
        llvm::Function::InternalLinkage, "cfc_body", module);
    {
        auto *bodyEntry = llvm::BasicBlock::Create(m_context, "entry", bodyFn);
        builder.SetInsertPoint(bodyEntry);
        auto *bodyCleanupBB = llvm::BasicBlock::Create(m_context, "body.cleanup", bodyFn);
        ScopedCodegenState<llvm::BasicBlock*> bodyCleanupBBGuard(g_currentFuncCleanupBB, bodyCleanupBB);

        auto *fCleanupSave = getOrCreateHelper(module, builder, "cfvariant_cleanup_save", builder.getInt64Ty(), {});
        llvm::Value *savepoint = builder.CreateCall(fCleanupSave, {});

        // Push this .cfc's call-stack frame (its construction body is a frame
        // of its own, like CF's component construction context).
        emitStackPush(module, builder);

        llvm::Value *out = bodyFn->getArg(0);
        llvm::Value *cgi = bodyFn->getArg(1);
        llvm::Value *server = bodyFn->getArg(2);
        llvm::Value *cookie = bodyFn->getArg(3);
        llvm::Value *application = bodyFn->getArg(4);
        llvm::Value *session = bodyFn->getArg(5);
        llvm::Value *url = bodyFn->getArg(6);
        llvm::Value *form = bodyFn->getArg(7);
        llvm::Value *variablesScope = bodyFn->getArg(8);
        llvm::Value *thisScope = bodyFn->getArg(9);
        (void)thisScope;

        WsFlag bodyFlag;
        WhitespaceState wsBody(config::enableWhitespaceManagement, bodyFlag);
        wsBody.markTag(false, false);

        std::vector<LoopInfo> loopStack;
        if (!isInterface) {
            if (scriptForm) {
                // Script-form construction body: compile the body statements (with
                // method/property declarations already stripped) directly as script.
                compile_script_expression(construction, m_context, module, builder, bodyFn,
                                          out, cgi, server, cookie, application, session, url, form, variablesScope,
                                          cfm_text, cfm_text_size, loopStack);
            } else {
                // The construction body is the token slice between <cfcomponent> and
                // </cfcomponent>; the cffunction/cfproperty blocks inside are skipped
                // by compile_token_list (they are collected separately above).
                std::vector<TextParserTokenItem> bodyTokens(tokens.begin() + compStart + 1,
                                                            tokens.begin() + compEnd);
                size_t index = 0;
                size_t pos = tokens[compStart].position + tokens[compStart].len;
                compile_token_list(bodyTokens, index, pos, m_context, module, builder, bodyFn,
                                   out, wsBody, cgi, server, cookie, application, session, url, form, variablesScope,
                                   cfm_text, cfm_text_size, loopStack);
                // Emit any trailing whitespace text up to the </cfcomponent> tag.
                size_t endPos = tokens[compEnd].position;
                if (pos < endPos) {
                    wsBody.feed(module, builder, out, cfm_text + pos, endPos - pos, WsRight::Tag);
                }
                wsBody.finish(module, builder, out, WsRight::Tag);
            }
        }

        auto *fRestore = getOrCreateHelper(module, builder, "cfvariant_cleanup_restore", builder.getVoidTy(), {builder.getInt64Ty()});
        builder.CreateCall(fRestore, {savepoint});
        emitStackPop(module, builder);
        builder.CreateRetVoid();

        builder.SetInsertPoint(bodyCleanupBB);
        auto *lpTy = llvm::StructType::get(m_context, {builder.getPtrTy(), builder.getInt32Ty()});
        auto *lp = builder.CreateLandingPad(lpTy, 0, "body.lp");
        lp->setCleanup(true);
        llvm::Value *bodyExn = builder.CreateExtractValue(lp, 0, "body.exn");
        emitStackCaptureOnException(module, builder, bodyExn);
        emitStackPop(module, builder);
        builder.CreateCall(fRestore, {savepoint});
        builder.CreateResume(lp);
    }

    // ---- Compile each method body (component_method_entry_fn) ----
    // Interface methods are declarations only (their signatures feed the
    // `implements` validation); no bodies are compiled.
    if (!isInterface) {
        for (size_t i = 0; i < methodDefs.size(); i++) {
            const UdfDef &def = methodDefs[i];
            std::string llvmName = "cfc_m_" + def.name;
            compileUdfFunction(module, m_context, builder, llvmName, def,
                               cfm_text, cfm_text_size, true);
        }
    }

    // Attach the C++ Itanium personality to every JIT function.
    {
        llvm::Function *pers = getOrCreatePersonality(module, builder, nullptr);
        for (auto &f : module->functions()) {
            if (!f.hasPersonalityFn()) f.setPersonalityFn(pers);
        }
    }

    // Generating code.
    std::string error;
    std::unique_ptr<llvm::ExecutionEngine> engine(llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module))
                      .setErrorStr(&error)
                      .setEngineKind(llvm::EngineKind::JIT)
                      .create());
    if (!engine.get()) {
        component_info_release(info);
        throw webstrada::exception("Code generation error!", "Failed to create EngineBuilder.");
    }
    engine.get()->finalizeObject();

    info->body = engine.get()->getPointerToNamedFunction("cfc_body");
    if (!isInterface) {
        for (size_t i = 0; i < methodDefs.size(); i++) {
            std::string llvmName = "cfc_m_" + methodDefs[i].name;
            void *p = engine.get()->getPointerToNamedFunction(llvmName);
            info->methods[i].fn = p;
        }
    }

    m_engines.push_back(std::move(engine));
    auto t1 = std::chrono::steady_clock::now();
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    fprintf(stderr, "[WebStrada] compiled %s (%lldms)\n", pathname.constData(), ms);
    return info;
}


} // namespace webstrada
