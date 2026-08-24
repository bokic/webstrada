/**
 * @file llvm_codegen.cpp
 * Central LLVM tag/script/template token dispatcher + UDF codegen.
 * Expression / script / tag / token helpers live in src/codegen/.
 */

#include <webstrada/llvm_codegen.h>
#include "codegen_internal.h"
#include <webstrada/config.h>
#include <webstrada/exceptions.h>
#include <webstrada/parser.h>
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

#include <dlfcn.h>

#include <textparser.hpp>
#include <cfml_definition.json.h>

using namespace webstrada;

#include "codegen_internal.h"

bool webstrada::config::enableWhitespaceManagement = true;

namespace webstrada {

thread_local EhContext *g_ehContext = nullptr;
thread_local llvm::Value *g_currentCatchExn = nullptr;
thread_local llvm::Value *g_currentOut = nullptr;
thread_local llvm::BasicBlock *g_currentFuncCleanupBB = nullptr;
thread_local bool g_compileInFunctionBody = false;
thread_local textparser_t g_textparser = nullptr;
thread_local std::map<std::pair<llvm::Function*, std::string>, llvm::Value*> g_varFastSlots;
int g_closureCounter = 0;
FunctionReturnCtx *g_returnCtx = nullptr;

// ---- forward declarations (same-file) ----

void compile_token_list(
    const std::vector<TextParserTokenItem> &tokens,
    size_t &index,
    size_t &pos,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
    WhitespaceState &ws,
    llvm::Value *cgi,
    llvm::Value *server,
    llvm::Value *cookie,
    llvm::Value *application,
    llvm::Value *session,
    llvm::Value *url,
    llvm::Value *form,
    llvm::Value *variables,
    const char *cfm_text,
    size_t cfm_text_size,
    std::vector<LoopInfo> &loopStack);

int lineOfOffset(const char *text, size_t pos)
{
    // Prefer the textparser line map: authoritative and encoding-independent
    // (positions are byte offsets, so counting '\n' by hand is equivalent only
    // for single-byte encodings).
    if (g_textparser) {
        return static_cast<int>(textparser_get_line_number_at_position(g_textparser, pos)) + 1;
    }
    // Fallback (no active compile): 1-based line number of the byte offset
    // `pos` (1 + count of '\n' before pos); 1 when pos is out of range.
    if (!text || pos == 0) return 1;
    int line = 1;
    size_t n = pos < strlen(text) ? pos : strlen(text);
    for (size_t i = 0; i < n; i++) {
        if (text[i] == '\n') line++;
    }
    return line;
}

void emitStackPush(llvm::Module *module, llvm::IRBuilder<> &builder, const char *functionName)
{
    llvm::StringRef moduleName = module->getName();
    auto *fPush = getOrCreateHelper(module, builder, "cf_stack_push", builder.getVoidTy(),
                                    {builder.getPtrTy(), builder.getPtrTy()});
    // Plain call (not invoke): these helpers cannot throw, and a call inside a
    // landing pad that unwinds to the same pad would self-loop and break LLVM's
    // pass pipeline (ConstantHoistingPass crash).
    builder.CreateCall(fPush, {
        builder.CreateGlobalString(moduleName, "", 0, module, true),
        builder.CreateGlobalString(llvm::StringRef(functionName ? functionName : ""), "", 0, module, true)});
}

void emitStackSetLine(llvm::Module *module, llvm::IRBuilder<> &builder, int line)
{
    auto *fSetLine = getOrCreateHelper(module, builder, "cf_stack_set_line", builder.getVoidTy(),
                                       {builder.getInt32Ty()});
    builder.CreateCall(fSetLine, {builder.getInt32(line)});
}

void emitStackPop(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto *fPop = getOrCreateHelper(module, builder, "cf_stack_pop", builder.getVoidTy(), {});
    builder.CreateCall(fPop, {});
}

void emitStackCaptureOnException(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *exn)
{
    auto *fCapture = getOrCreateHelper(module, builder, "cf_stack_capture_on_exception",
                                       builder.getVoidTy(), {builder.getPtrTy()});
    builder.CreateCall(fCapture, {exn});
}

// ---- definitions ----

size_t parseFunctionDecl(const std::vector<TextParserTokenItem> &tokens, size_t start,
                                const char *cfm_text, UdfDef &def)
{
    def = UdfDef();
    if (start + 1 < tokens.size() && tokens[start + 1].token_id == TextParser_cfml_Function) {
        def.name = tokenText(tokens[start + 1], cfm_text);
    }
    size_t idx = start + 2;
    if (idx < tokens.size() && tokens[idx].token_id == TextParser_cfml_Parenthesis) {
        parseParamList(tokens[idx], cfm_text, def.paramNames, def.paramTypes, def.paramDefaults);
        idx++;
    }
    // Attributes: returntype="numeric", output="false", ... as Variable =
    // DoubleString triples between the parens and the body.
    while (idx + 2 < tokens.size() &&
           tokens[idx].token_id == TextParser_cfml_Variable &&
           isOperatorToken(tokens[idx + 1].token_id)) {
        std::string attrName = tokenText(tokens[idx], cfm_text);
        std::string attrUpper;
        for (auto &c : attrName) attrUpper.push_back((char)toupper((unsigned char)c));
        if (attrUpper == "RETURNTYPE" && tokens[idx + 2].token_id == TextParser_cfml_DoubleString) {
            std::string v = tokenText(tokens[idx + 2], cfm_text);
            if (v.size() >= 2) v = v.substr(1, v.size() - 2);
            def.returnType = v;
        }
        idx += 3;
        while (idx < tokens.size() &&
               (tokens[idx].token_id == TextParser_cfml_ScriptLineComment ||
                tokens[idx].token_id == TextParser_cfml_ScriptBlockComment)) {
            idx++;
        }
    }
    while (idx < tokens.size() &&
           (tokens[idx].token_id == TextParser_cfml_ScriptLineComment ||
            tokens[idx].token_id == TextParser_cfml_ScriptBlockComment)) {
        idx++;
    }
    if (idx < tokens.size() && tokens[idx].token_id == TextParser_cfml_CodeBlock) {
        const auto &cb = tokens[idx];
        if (!cb.children.empty() && cb.children[0].token_id == TextParser_cfml_ScriptExpression) {
            def.bodyTokens = cb.children[0].children;
        } else {
            def.bodyTokens = cb.children;
        }
        idx++;
    }
    return idx;
}

std::string buildUdfMetaBlob(const UdfDef &def, const char *cfm_text)
{
    UdfMetaInfo meta;
    for (size_t i = 0; i < def.paramNames.size(); i++) {
        UdfParamInfo p;
        p.name = string(def.paramNames[i].c_str());
        p.type = string(def.paramTypes[i].c_str());
        if (i < def.paramDefaults.size() && !def.paramDefaults[i].empty()) {
            const auto &toks = def.paramDefaults[i];
            if (toks.size() == 1) {
                const auto &t = toks[0];
                if (t.token_id == TextParser_cfml_Number || t.token_id == TextParser_cfml_Boolean) {
                    p.defaultValue = string(tokenText(t, cfm_text).c_str());
                } else if (t.token_id == TextParser_cfml_DoubleString || t.token_id == TextParser_cfml_SingleString) {
                    std::string s = tokenText(t, cfm_text);
                    if (s.size() >= 2) s = s.substr(1, s.size() - 2);
                    p.defaultValue = string(s.c_str());
                }
            }
            if (p.defaultValue.isEmpty()) {
                std::string s;
                for (const auto &t : toks) s += tokenText(t, cfm_text);
                p.defaultValue = string(s.c_str());
            }
        }
        meta.params.push_back(std::move(p));
    }
    meta.returnType = string(def.returnType.c_str());
    meta.access = "public";
    return udf_meta_serialize(meta);
}

void collectFunctionDecls(const std::vector<TextParserTokenItem> &tokens,
                                 const char *cfm_text, std::vector<UdfDef> &out)
{
    size_t i = 0;
    while (i < tokens.size()) {
        const auto &t = tokens[i];
        if (t.token_id == TextParser_cfml_Keyword && kwTextIs(t, cfm_text, "function")) {
            if (i + 1 < tokens.size() && tokens[i + 1].token_id == TextParser_cfml_Function) {
                UdfDef def;
                i = parseFunctionDecl(tokens, i, cfm_text, def);
                out.push_back(std::move(def));
                continue;
            }
            if (i + 2 < tokens.size() &&
                tokens[i + 1].token_id == TextParser_cfml_Parenthesis &&
                tokens[i + 2].token_id == TextParser_cfml_CodeBlock) {
                i += 3; // closure header + body: closure-scoped, skip
                continue;
            }
            i++;
            continue;
        }
        if (t.token_id == TextParser_cfml_CodeBlock) {
            for (const auto &child : t.children) {
                if (child.token_id == TextParser_cfml_ScriptExpression) {
                    collectFunctionDecls(child.children, cfm_text, out);
                }
            }
        }
        i++;
    }
}

static void scanFunctionBody(const std::vector<TextParserTokenItem> &tokens,
                             const char *cfm_text,
                             std::vector<UdfDef> &nested,
                             std::set<std::string> &varNames)
{
    size_t i = 0;
    while (i < tokens.size()) {
        const auto &t = tokens[i];
        if (t.token_id == TextParser_cfml_Keyword && kwTextIs(t, cfm_text, "var") &&
            i + 1 < tokens.size() && tokens[i + 1].token_id == TextParser_cfml_Variable) {
            std::string v = tokenText(tokens[i + 1], cfm_text);
            for (auto &c : v) c = (char)toupper((unsigned char)c);
            varNames.insert(v);
            i += 2;
            continue;
        }
        if (t.token_id == TextParser_cfml_Keyword && kwTextIs(t, cfm_text, "function")) {
            if (i + 1 < tokens.size() && tokens[i + 1].token_id == TextParser_cfml_Function) {
                UdfDef def;
                i = parseFunctionDecl(tokens, i, cfm_text, def);
                std::string uname = def.name;
                for (auto &c : uname) c = (char)toupper((unsigned char)c);
                varNames.insert(uname);
                nested.push_back(std::move(def));
            } else if (i + 2 < tokens.size() &&
                       tokens[i + 1].token_id == TextParser_cfml_Parenthesis &&
                       tokens[i + 2].token_id == TextParser_cfml_CodeBlock) {
                i += 3; // closure: its var-scope is separate
            } else {
                i++;
            }
            continue;
        }
        if (t.token_id == TextParser_cfml_CodeBlock) {
            for (const auto &child : t.children) {
                if (child.token_id == TextParser_cfml_ScriptExpression) {
                    scanFunctionBody(child.children, cfm_text, nested, varNames);
                }
            }
        }
        i++;
    }
}

size_t parseTagFunctionDecl(const std::vector<TextParserTokenItem> &tokens,
                                   size_t start, const char *cfm_text, UdfDef &def)
{
    def = UdfDef();
    def.isTagForm = true;
    std::map<std::string, std::string> attrs;
    parseTagAttrs(tokens[start], cfm_text, attrs);
    auto nameIt = attrs.find("name");
    if (nameIt != attrs.end()) def.name = nameIt->second;
    auto rtIt = attrs.find("returntype");
    if (rtIt != attrs.end()) def.returnType = rtIt->second;
    auto outIt = attrs.find("output");
    if (outIt != attrs.end()) {
        std::string ov = outIt->second;
        for (auto &c : ov) c = (char)tolower((unsigned char)c);
        def.output = !(ov == "false" || ov == "no");
    }
    auto accIt = attrs.find("access");
    if (accIt != attrs.end()) def.access = accIt->second;

    // Scan forward to the matching `</cffunction>` (tracking nested
    // <cffunction> depth; cffunction must not nest but a stray one is ignored).
    size_t endTagIdx = tokens.size();
    {
        int depth = 0;
        for (size_t i = start + 1; i < tokens.size(); i++) {
            const auto &tok = tokens[i];
            if (tok.token_id == TextParser_cfml_StartTag && tagNameOf(tok, cfm_text) == "cffunction") {
                depth++;
            } else if (tok.token_id == TextParser_cfml_EndTag && tagNameOf(tok, cfm_text) == "cffunction") {
                if (depth > 0) depth--;
                else { endTagIdx = i; break; }
            }
        }
    }
    if (endTagIdx >= tokens.size()) {
        throw webstrada::exception("cffunction", "Missing closing </cffunction> tag");
    }
    def.bodyStart = tokens[start].position + tokens[start].len;
    def.bodyEnd = tokens[endTagIdx].position;

    // Collect `<cfargument>` declarations at the body's top level (generic tag
    // nesting depth 0, so one inside a cfif is not mistaken for a direct
    // child). Defaults are kept as raw attribute strings (paramDefaultsRaw) and
    // compiled at the call site like script-form defaults.
    {
        int depth = 0;
        for (size_t i = start + 1; i < endTagIdx; i++) {
            const auto &tok = tokens[i];
            if (tok.token_id == TextParser_cfml_StartTag) {
                std::string tn = tagNameOf(tok, cfm_text);
                if (tn == "cfargument") {
                    if (depth == 0) {
                        std::map<std::string, std::string> aattrs;
                        parseTagAttrs(tok, cfm_text, aattrs);
                        auto anIt = aattrs.find("name");
                        def.paramNames.push_back(anIt != aattrs.end() ? anIt->second : "");
                        auto tIt = aattrs.find("type");
                        def.paramTypes.push_back(tIt != aattrs.end() ? tIt->second : "");
                        auto rIt = aattrs.find("required");
                        bool req = false;
                        if (rIt != aattrs.end()) {
                            std::string rv = rIt->second;
                            for (auto &c : rv) c = (char)tolower((unsigned char)c);
                            req = (rv == "true" || rv == "yes");
                        }
                        def.paramRequired.push_back(req);
                        auto dIt = aattrs.find("default");
                        def.paramDefaultsRaw.push_back(dIt != aattrs.end() ? dIt->second : "");
                        def.paramHasDefaultRaw.push_back(dIt != aattrs.end());
                        def.paramDefaults.emplace_back();
                    }
                } else if (tn == "cffunction") {
                    depth++;
                }
            } else if (tok.token_id == TextParser_cfml_EndTag && tagNameOf(tok, cfm_text) == "cffunction") {
                if (depth > 0) depth--;
            }
        }
    }

    def.bodyTokens.assign(tokens.begin() + start + 1, tokens.begin() + endTagIdx);
    return endTagIdx + 1;
}

void collectTagFunctionDecls(const std::vector<TextParserTokenItem> &tokens,
                                    const char *cfm_text, std::vector<UdfDef> &out)
{
    size_t i = 0;
    while (i < tokens.size()) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag && tagNameOf(tok, cfm_text) == "cffunction") {
            UdfDef def;
            i = parseTagFunctionDecl(tokens, i, cfm_text, def);
            out.push_back(std::move(def));
            continue;
        }
        i++;
    }
}

static void scanTagFunctionBody(const std::vector<TextParserTokenItem> &tokens,
                                const char *cfm_text,
                                std::vector<UdfDef> &nested,
                                std::set<std::string> &varNames)
{
    size_t i = 0;
    while (i < tokens.size()) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag) {
            std::string tn = tagNameOf(tok, cfm_text);
            if (tn == "cfset") {
                for (const auto &ch : tok.children) {
                    if (ch.token_id != TextParser_cfml_Expression) continue;
                    for (size_t ci = 0; ci + 1 < ch.children.size(); ci++) {
                        const auto &ct = ch.children[ci];
                        if (ct.token_id == TextParser_cfml_Keyword && kwTextIs(ct, cfm_text, "var") &&
                            ch.children[ci + 1].token_id == TextParser_cfml_Variable) {
                            std::string v = tokenText(ch.children[ci + 1], cfm_text);
                            for (auto &c : v) c = (char)toupper((unsigned char)c);
                            varNames.insert(v);
                        }
                    }
                }
            } else if (tn == "cffunction") {
                UdfDef def;
                i = parseTagFunctionDecl(tokens, i, cfm_text, def);
                std::string uname = def.name;
                for (auto &c : uname) c = (char)toupper((unsigned char)c);
                varNames.insert(uname);
                nested.push_back(std::move(def));
                continue;
            }
        } else if (tok.token_id == TextParser_cfml_ScriptTagPair) {
            for (const auto &ch : tok.children) {
                if (ch.token_id == TextParser_cfml_ScriptExpression) {
                    scanFunctionBody(ch.children, cfm_text, nested, varNames);
                }
            }
        }
        i++;
    }
}

// True when the token at `vi` of a tag's attribute parts starts a NEW attribute
// (a `name=` pair): a Variable token, or the `var` keyword form (`var=`). The
// textparser tokenizes `var` as a Keyword, so without this check `var="x"`
// would be swallowed into the previous attribute's value tokens (see BUGS.md
// "var cannot be used as a named function argument").
static bool isAttrNameToken(const std::vector<TextParserTokenItem> &parts,
                            size_t vi, const char *cfm_text)
{
    const auto &vt = parts[vi];
    if (vt.token_id == TextParser_cfml_Variable) return true;
    if (vt.token_id == TextParser_cfml_Keyword && vt.len == 3 &&
        std::memcmp(cfm_text + vt.position, "var", 3) == 0) return true;
    return false;
}

static llvm::Function *getUdfSignatureFn(llvm::Module *module, llvm::IRBuilder<> &builder,
                                         const std::string &name, bool isComponentMethod = false)
{
    auto *existing = module->getFunction(name);
    if (existing) return existing;
    std::vector<llvm::Type*> params(8, builder.getPtrTy());
    if (isComponentMethod) {
        // component_method_entry_fn: (out, cgi, ..., form, variablesScope,
        // thisScope, component, args, argc)
        params.push_back(builder.getPtrTy()); // variablesScope
        params.push_back(builder.getPtrTy()); // thisScope
        params.push_back(builder.getPtrTy()); // component
        params.push_back(builder.getPtrTy()); // args
        params.push_back(builder.getInt32Ty()); // argc
    } else {
        params.push_back(builder.getPtrTy()); // parentScope
        params.push_back(builder.getPtrTy()); // args
        params.push_back(builder.getInt32Ty()); // argc
    }
    return llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), params, false),
                                  llvm::Function::InternalLinkage, name, module);
}

llvm::Function *getOrCreatePersonality(llvm::Module *module, llvm::IRBuilder<> &builder,
                                              llvm::Function *fn)
{
    llvm::Function *pers = module->getFunction("__gxx_personality_v0");
    if (!pers) {
        auto persTy = llvm::FunctionType::get(
            builder.getInt32Ty(),
            {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt64Ty(),
             builder.getPtrTy(), builder.getPtrTy()},
            true);
        pers = llvm::Function::Create(persTy, llvm::Function::ExternalLinkage,
                                      "__gxx_personality_v0", module);
    }
    if (fn && !fn->hasPersonalityFn()) fn->setPersonalityFn(pers);
    return pers;
}

llvm::Function *compileUdfFunction(
    llvm::Module *module,
    llvm::LLVMContext &context,
    llvm::IRBuilder<> &builder,
    const std::string &llvmName,
    const UdfDef &def,
    const char *cfm_text,
    size_t cfm_text_size,
    bool isComponentMethod,
    const char *stackFnName)
{
    // Save the caller's insert point: compiling a UDF body moves the builder
    // into the new function, and the caller must continue where it left off.
    auto savedIP = builder.saveIP();

    llvm::Function *fn = getUdfSignatureFn(module, builder, llvmName, isComponentMethod);
    auto *entry = llvm::BasicBlock::Create(context, "udf.entry", fn);
    builder.SetInsertPoint(entry);

    auto *funcCleanupBB = llvm::BasicBlock::Create(context, "udf.cleanup", fn);
    ScopedCodegenState<llvm::BasicBlock*> cleanupBBGuard(g_currentFuncCleanupBB, funcCleanupBB);

    auto *fCleanupSave = getOrCreateHelper(module, builder, "cfvariant_cleanup_save", builder.getInt64Ty(), {});
    llvm::Value *frameSavepoint = builder.CreateCall(fCleanupSave, {});

    // Push this function's call-stack frame (the module name is the template /
    // .cfc pathname it was compiled from, carrying the uppercased function
    // name so CallStackGet/CallStackDump report it); popped on both exits.
    // `stackFnName` overrides def.name for anonymous closures (CF names them
    // "_CF_ANONYMOUSCLOSURE_<n>").
    const char *fnName = stackFnName ? stackFnName : def.name.c_str();
    emitStackPush(module, builder, fnName);

    llvm::Value *out = fn->getArg(0);
    llvm::Value *cgi = fn->getArg(1);
    llvm::Value *server = fn->getArg(2);
    llvm::Value *cookie = fn->getArg(3);
    llvm::Value *application = fn->getArg(4);
    llvm::Value *session = fn->getArg(5);
    llvm::Value *url = fn->getArg(6);
    llvm::Value *form = fn->getArg(7);
    llvm::Value *parentScope = fn->getArg(8);
    llvm::Value *argsPtr = nullptr;
    llvm::Value *argcVal = nullptr;
    llvm::Value *variablesScope = nullptr;
    llvm::Value *thisScope = nullptr;
    llvm::Value *componentInst = nullptr;
    if (isComponentMethod) {
        variablesScope = parentScope;
        thisScope = fn->getArg(9);
        componentInst = fn->getArg(10);
        argsPtr = fn->getArg(11);
        argcVal = fn->getArg(12);
    } else {
        argsPtr = fn->getArg(9);
        argcVal = fn->getArg(10);
    }

    auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
    auto *fRegisterTemp = getOrCreateHelper(module, builder, "cf_udf_register_temp", builder.getVoidTy(), {builder.getPtrTy()});
    auto *fBegin = isComponentMethod
        ? getOrCreateHelper(module, builder, "cf_component_udf_begin", builder.getVoidTy(),
                            {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()})
        : getOrCreateHelper(module, builder, "cf_udf_begin", builder.getVoidTy(),
                            {builder.getPtrTy(), builder.getPtrTy()});
    auto *fMarkLocal = getOrCreateHelper(module, builder, "cf_udf_mark_local", builder.getVoidTy(), {builder.getPtrTy()});
    auto *fAssign = getOrCreateHelper(module, builder, "cfvariant_assign", builder.getPtrTy(),
                                      {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                       builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                       builder.getPtrTy(), builder.getPtrTy()});
    auto *fBuildArgs = getOrCreateHelper(module, builder, "cf_udf_build_arguments", builder.getVoidTy(),
                                         {builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty(), builder.getPtrTy(), builder.getInt32Ty()});
    auto *fCoerceArg = getOrCreateHelper(module, builder, "cf_udf_coerce_arg", builder.getPtrTy(),
                                         {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
    auto *fCreateUdf = getOrCreateHelper(module, builder, "cfvariant_create_udf", builder.getPtrTy(),
                                         {builder.getPtrTy(), builder.getPtrTy(), builder.getInt1Ty(), builder.getPtrTy(), builder.getPtrTy()});

    // Prologue: fresh local scope, context push, local-name marking (params + var + nested + ARGUMENTS).
    llvm::Value *localScope = builder.CreateCall(fCreateStruct, {});
    if (isComponentMethod) {
        // cf_component_udf_begin(localScope, variablesScope, thisScope, component)
        emitCall(builder, fBegin, {localScope, variablesScope, thisScope, componentInst});
    } else {
        emitCall(builder, fBegin, {localScope, parentScope});
    }

    std::set<std::string> localNames;
    std::vector<UdfDef> nestedDefs;
    if (def.isTagForm) {
        scanTagFunctionBody(def.bodyTokens, cfm_text, nestedDefs, localNames);
    } else {
        scanFunctionBody(def.bodyTokens, cfm_text, nestedDefs, localNames);
    }
    // CF: `local` holds only var-declared names (+ ARGUMENTS), NOT the function
    // arguments (which live in `arguments`); a `var` name may not collide with a
    // parameter ("ARG1 is already defined in argument scope."). Params are bound
    // into the local scope only temporarily (so cf_udf_build_arguments can copy
    // them) and removed via cf_udf_remove_params before the body runs.
    for (const auto &p : def.paramNames) {
        std::string u = p;
        for (auto &c : u) c = (char)toupper((unsigned char)c);
        if (localNames.count(u)) {
            throw webstrada::exception(webstrada::string((u + " is already defined in argument scope.").c_str()));
        }
    }
    localNames.insert("ARGUMENTS");
    for (const auto &n : localNames) {
        emitCall(builder, fMarkLocal, {builder.CreateGlobalString(llvm::StringRef(n), "", 0, module, true)});
    }

    // Bind parameters: positional args in order; missing params get their
    // default (compiled inline) or stay undefined (not bound).
    for (size_t i = 0; i < def.paramNames.size(); i++) {
        std::string uname = def.paramNames[i];
        for (auto &c : uname) c = (char)toupper((unsigned char)c);
        llvm::Value *value = nullptr;
        llvm::Value *argPtr = builder.CreateGEP(builder.getPtrTy(), argsPtr, builder.getInt32(static_cast<int>(i)));
        llvm::Value *arg = builder.CreateLoad(builder.getPtrTy(), argPtr);
        llvm::Value *argcCond = builder.CreateICmpSGT(argcVal, builder.getInt32(static_cast<int>(i)));
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "param.then", fn);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "param.else", fn);
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "param.merge", fn);
        builder.CreateCondBr(argcCond, thenBB, elseBB);
        builder.SetInsertPoint(thenBB);
        // A NotSet arg marks a missing named/positional parameter (a gap left by
        // named-argument reordering); treat it as not-passed so the default is
        // used instead of binding an unset value. Only read the type when the
        // arg is actually present (argc > i), so a positional call with fewer
        // args never reads past the end of the args array.
        if (arg && arg->getType() == builder.getPtrTy()) {
            auto *fType = getOrCreateHelper(module, builder, "cfvariant_type", builder.getInt32Ty(), {builder.getPtrTy()});
            llvm::Value *typeVal = emitCall(builder, fType, {arg});
            llvm::Value *notSetCond = builder.CreateICmpEQ(typeVal, builder.getInt32(static_cast<int>(cfvariant::NotSet)));
            auto *notSetBB = llvm::BasicBlock::Create(context, "param.notset", fn);
            auto *presentBB = llvm::BasicBlock::Create(context, "param.present", fn);
            builder.CreateCondBr(notSetCond, notSetBB, presentBB);
            builder.SetInsertPoint(notSetBB);
            builder.CreateBr(elseBB);
            builder.SetInsertPoint(presentBB);
        }
        llvm::Value *thenVal = arg;
        if (!def.paramTypes[i].empty()) {
            thenVal = emitCall(builder, fCoerceArg, {arg, builder.CreateGlobalString(llvm::StringRef(def.paramTypes[i]), "", 0, module, true),
                                                      builder.CreateGlobalString(llvm::StringRef(def.paramNames[i]), "", 0, module, true),
                                                      builder.CreateGlobalString(llvm::StringRef(def.name), "", 0, module, true)});
        }
        // Bind the parameter into the local scope DIRECTLY (cfvariant_assign
        // would route a param name to the arguments scope once params are not
        // marked local). cf_udf_build_arguments copies these slots into the
        // `arguments` struct, and cf_udf_remove_params erases them afterwards.
        auto *fIdxAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                             {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
        auto *fStrKey = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
        emitCall(builder, fIdxAssign, {localScope,
                                        emitCall(builder, fStrKey, {builder.CreateGlobalString(llvm::StringRef(uname), "", 0, module, true)}),
                                        thenVal});
        builder.CreateBr(mergeBB);
        builder.SetInsertPoint(elseBB);
        bool hasDefault = def.isTagForm
            ? (i < def.paramHasDefaultRaw.size() && def.paramHasDefaultRaw[i])
            : (i < def.paramDefaults.size() && !def.paramDefaults[i].empty());
        bool required = (i < def.paramRequired.size() && def.paramRequired[i]) && !hasDefault;
        if (required) {
            auto *fThrow = getOrCreateHelper(module, builder, "cf_throw_missing_argument", builder.getVoidTy(),
                                             {builder.getPtrTy(), builder.getPtrTy()});
            emitCall(builder, fThrow, {
                builder.CreateGlobalString(llvm::StringRef(def.paramNames[i]), "", 0, module, true),
                builder.CreateGlobalString(llvm::StringRef(def.name), "", 0, module, true)});
            builder.CreateUnreachable();
        } else {
            if (hasDefault) {
                llvm::Value *defVal = nullptr;
                if (def.isTagForm) {
                    // CF tag-attribute semantics: a quoted `default="..."` is a
                    // literal string unless it is a `#...#`-wrapped expression,
                    // in which case it is evaluated (verified on CF:
                    // default="1+1" -> the string "1+1", default="#1+1#" -> 2).
                    std::string raw = def.paramDefaultsRaw[i];
                    size_t ls = raw.find_first_not_of(" \t\r\n");
                    size_t le = raw.find_last_not_of(" \t\r\n");
                    if (ls == std::string::npos) raw = "";
                    else raw = raw.substr(ls, le - ls + 1);
                    if (raw.size() >= 2 && raw.front() == '#' && raw.back() == '#') {
                        std::string inner = raw.substr(1, raw.size() - 2);
                        defVal = CompileStringExpression(module, builder, fn, cgi, server, cookie, application, session, url, form, isComponentMethod ? variablesScope : localScope, string(inner.c_str()), cfm_text);
                    } else {
                        auto *fStr = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                        defVal = emitCall(builder, fStr, {builder.CreateGlobalString(llvm::StringRef(raw), "", 0, module, true)});
                    }
                } else {
                    auto defaultAst = parseTokensToAST(def.paramDefaults[i], cfm_text);
                    defVal = CompileExprAST(module, builder, fn, defaultAst, cgi, server, cookie, application, session, url, form, isComponentMethod ? variablesScope : localScope, cfm_text);
                }
                auto *fIdxAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                                     {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                auto *fStrKey = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                emitCall(builder, fIdxAssign, {localScope,
                                                emitCall(builder, fStrKey, {builder.CreateGlobalString(llvm::StringRef(uname), "", 0, module, true)}),
                                                defVal});
            }
            builder.CreateBr(mergeBB);
        }
        builder.SetInsertPoint(mergeBB);
    }

    // Build the `arguments` scope and register nested functions into the local scope.
    {
        std::vector<llvm::Value*> argNamesPtrs;
        for (const auto &p : def.paramNames) {
            argNamesPtrs.push_back(builder.CreateGlobalString(llvm::StringRef(p), "", 0, module, true));
        }
        llvm::Value *namesArray = nullptr;
        if (!argNamesPtrs.empty()) {
            namesArray = createEntryAlloca(builder, fn, builder.getPtrTy(), builder.getInt32(static_cast<int>(argNamesPtrs.size())));
            for (size_t i = 0; i < argNamesPtrs.size(); i++) {
                auto *ptr = builder.CreateGEP(builder.getPtrTy(), namesArray, builder.getInt32(static_cast<int>(i)));
                builder.CreateStore(argNamesPtrs[i], ptr);
            }
        } else {
            namesArray = llvm::ConstantPointerNull::get(builder.getPtrTy());
        }
        emitCall(builder, fBuildArgs, {localScope, namesArray, builder.getInt32(static_cast<int>(def.paramNames.size())),
                                        argsPtr, argcVal});
        // Parameters are not keys of the `local` scope (CF); the arguments
        // struct above already captured their values, so remove them now that
        // the body is about to run.
        if (!def.paramNames.empty()) {
            auto *fRemoveParams = getOrCreateHelper(module, builder, "cf_udf_remove_params", builder.getVoidTy(),
                                                    {builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty()});
            emitCall(builder, fRemoveParams, {localScope, namesArray, builder.getInt32(static_cast<int>(def.paramNames.size()))});
        }
        for (const auto &nested : nestedDefs) {
            llvm::Function *nestedFn = compileUdfFunction(module, context, builder, "udf_" + nested.name + "_nested_" + std::to_string(g_closureCounter++), nested, cfm_text, cfm_text_size);
            std::string uname = nested.name;
            for (auto &c : uname) c = (char)toupper((unsigned char)c);
            std::string nestedMeta = buildUdfMetaBlob(nested, cfm_text);
            llvm::Value *udfVal = emitCall(builder, fCreateUdf, {
                builder.CreateGlobalString(llvm::StringRef(nested.name), "", 0, module, true),
                nestedFn, builder.getInt1(false), localScope,
                builder.CreateGlobalString(llvm::StringRef(nestedMeta.data(), nestedMeta.size()), "", 0, module, true)});
            emitCall(builder, fAssign, {cgi, server, cookie, application, session, url, form, localScope,
                                         builder.CreateGlobalString(llvm::StringRef(uname), "", 0, module, true), udfVal});
        }
    }

    // Compile the body.
    llvm::AllocaInst *retSlot = createEntryAlloca(builder, fn, builder.getPtrTy());
    auto *fNull = getOrCreateHelper(module, builder, "cfvariant_create_null", builder.getPtrTy(), {});
    builder.CreateStore(emitCall(builder, fNull, {}), retSlot);
    auto *exitBB = llvm::BasicBlock::Create(context, "udf.exit", fn);
    FunctionReturnCtx rctx;
    rctx.retSlot = retSlot;
    rctx.exitBB = exitBB;
    rctx.returnType = def.returnType;
    rctx.funcName = def.name;
    ScopedCodegenState<FunctionReturnCtx*> returnCtxGuard(g_returnCtx, &rctx);

    std::vector<LoopInfo> loopStack;
    if (def.isTagForm) {
        // Tag-form body: compiled as CFML tags (text + StartTag/EndTag/...).
        // output="false" redirects the body's output to a runtime discard
        // buffer (cf_silent_begin/end, the same mechanism as <cfsilent>);
        // output="true" (the default) writes straight into the caller's output
        // buffer. The body uses its own whitespace flag so the page's whitespace
        // management is unaffected (verified against CF: a function's internal
        // <cfoutput> does not arm the caller's space flag).
        WsFlag bodyFlag;
        WhitespaceState wsBody(config::enableWhitespaceManagement, bodyFlag);
        wsBody.markTag(false, false); // left neighbour is <cffunction>
        if (def.output) {
            // For output-enabled functions the leading whitespace region of the
            // body collapses to a single space (verified on CF: `[ K ]` for a
            // body that starts with `\n    <cfoutput>K`); output="false"
            // discards it with everything else.
            bodyFlag.arm = true;
        }
        llvm::Value *bodyOut = out;
        llvm::Value *discard = nullptr;
        if (!def.output) {
            auto *fBegin = module->getFunction("cf_silent_begin");
            if (!fBegin) fBegin = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                                                         llvm::Function::InternalLinkage, "cf_silent_begin", module);
            auto *fEnd = module->getFunction("cf_silent_end");
            if (!fEnd) fEnd = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {}, false),
                                                     llvm::Function::InternalLinkage, "cf_silent_end", module);
            discard = emitCall(builder, fBegin, {out});
            bodyOut = discard;
        }
        size_t bodyPos = def.bodyStart;
        size_t bidx = 0;
        llvm::Value *bodyVariables = isComponentMethod ? variablesScope : localScope;
        {
            ScopedCodegenState<bool> inFuncGuard(g_compileInFunctionBody, true);
            compile_token_list(def.bodyTokens, bidx, bodyPos, context, module, builder, fn,
                               bodyOut, wsBody, cgi, server, cookie, application, session, url, form, bodyVariables,
                               cfm_text, def.bodyEnd, loopStack);
        }
        if (bodyPos < def.bodyEnd) {
            wsBody.feed(module, builder, bodyOut, cfm_text + bodyPos, def.bodyEnd - bodyPos, WsRight::Tag);
        }
        wsBody.finish(module, builder, bodyOut, WsRight::Tag);
    } else {
        {
            ScopedCodegenState<bool> inFuncGuard(g_compileInFunctionBody, true);
            compile_script_expression(def.bodyTokens, context, module, builder, fn, out, cgi, server, cookie, application, session, url, form,
                                      isComponentMethod ? variablesScope : localScope,
                                      cfm_text, cfm_text_size, loopStack);
        }
    }

    builder.CreateBr(exitBB);
    // Normal exit path: restore all temps except the return value
    builder.SetInsertPoint(exitBB);
    if (def.isTagForm && !def.output) {
        auto *fEnd = getOrCreateHelper(module, builder, "cf_silent_end", builder.getVoidTy(), {});
        builder.CreateCall(fEnd, {});
    }
    llvm::Value *retVal = builder.CreateLoad(builder.getPtrTy(), retSlot);
    auto *fUdfEnd = getOrCreateHelper(module, builder, "cf_udf_end", builder.getVoidTy(), {});
    builder.CreateCall(fUdfEnd, {});
    emitStackPop(module, builder);
    auto *fRestoreExcept = getOrCreateHelper(module, builder, "cfvariant_cleanup_restore_except",
                                             builder.getVoidTy(), {builder.getInt64Ty(), builder.getPtrTy()});
    builder.CreateCall(fRestoreExcept, {frameSavepoint, retVal});
    builder.CreateRet(retVal);

    // Exception unwinding path: restore all temps and resume exception
    builder.SetInsertPoint(funcCleanupBB);
    auto *lpTy = llvm::StructType::get(context, {builder.getPtrTy(), builder.getInt32Ty()});
    auto *lp = builder.CreateLandingPad(lpTy, 0, "udf.lp");
    lp->setCleanup(true);
    // Snapshot the stack into the in-flight exception (the first landing pad
    // that sees it), then pop this frame before resuming.
    llvm::Value *udfExn = builder.CreateExtractValue(lp, 0, "udf.exn");
    emitStackCaptureOnException(module, builder, udfExn);
    emitStackPop(module, builder);
    if (def.isTagForm && !def.output) {
        auto *fEnd = getOrCreateHelper(module, builder, "cf_silent_end", builder.getVoidTy(), {});
        builder.CreateCall(fEnd, {});
    }
    builder.CreateCall(fUdfEnd, {});
    auto *fRestore = getOrCreateHelper(module, builder, "cfvariant_cleanup_restore",
                                       builder.getVoidTy(), {builder.getInt64Ty()});
    builder.CreateCall(fRestore, {frameSavepoint});
    builder.CreateResume(lp);

    builder.restoreIP(savedIP);
    return fn;
}

void compile_token_list(
    const std::vector<TextParserTokenItem> &tokens,
    size_t &index,
    size_t &pos,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
    WhitespaceState &ws,
    llvm::Value *cgi,
    llvm::Value *server,
    llvm::Value *cookie,
    llvm::Value *application,
    llvm::Value *session,
    llvm::Value *url,
    llvm::Value *form,
    llvm::Value *variables,
    const char *cfm_text,
    size_t cfm_text_size,
    std::vector<LoopInfo> &loopStack) {
    auto cfevalboolFunc = get_cfevalbool_func(module, builder);
    auto lpSetFunc = module->getFunction("cfloop_set_long");
    if (!lpSetFunc) {
        std::vector<llvm::Type*> sp(3, builder.getPtrTy());
        sp[2] = builder.getInt64Ty();
        lpSetFunc = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), sp, false),
            llvm::Function::InternalLinkage, "cfloop_set_long", module);
    }

    ScopedCodegenState<llvm::Value*> currentOutGuard(g_currentOut, out);

    // Keep the top call-stack frame's line in sync as we execute: emit a
    // cf_stack_set_line before each executable construct (line numbers within a
    // source region are non-decreasing, so consecutive same-line tokens are
    // deduplicated). Recursive calls (if/loop/case/cfoutput bodies) update the
    // same top frame with their own tokens' lines.
    int lastStackLine = -1;

    while (index < tokens.size()) {
        const auto &token = tokens[index];

        // A CFML comment never executes, so it does not advance the line.
        if (token.token_id != TextParser_cfml_Comment) {
            int tokLine = lineOfOffset(cfm_text, token.position);
            if (tokLine != lastStackLine) {
                emitStackSetLine(module, builder, tokLine);
                lastStackLine = tokLine;
            }
        }

        // Tag validation check
        if (token.token_id == TextParser_cfml_StartTag ||
            token.token_id == TextParser_cfml_EndTag ||
            token.token_id == TextParser_cfml_QueryTagPair ||
            token.token_id == TextParser_cfml_QueryStartTag ||
            token.token_id == TextParser_cfml_QueryEndTag ||
            token.token_id == TextParser_cfml_LoopStartTag ||
            token.token_id == TextParser_cfml_LoopEndTag ||
            token.token_id == TextParser_cfml_OutputStartTag ||
            token.token_id == TextParser_cfml_OutputEndTag ||
            token.token_id == TextParser_cfml_ScriptStartTag ||
            token.token_id == TextParser_cfml_ScriptEndTag)
        {
            string tokenName = string(cfm_text + token.position, token.len);
            std::string s = tokenName.constData();
            size_t start = 0;
            if (s.rfind("</", 0) == 0) {
                start = 2;
            } else if (s.rfind("<", 0) == 0) {
                start = 1;
            }
            size_t end = start;
            while (end < s.length() && (isalnum(s[end]) || s[end] == '_')) {
                end++;
            }
            std::string tagName = s.substr(start, end - start);
            for (char &c : tagName) {
                c = tolower(c);
            }

            static const std::unordered_set<std::string> supported_tags = {
                "cfset", "cfdump", "cfif", "cfelseif", "cfelse", "cfswitch", "cfcase", "cfdefaultcase", 
                "cfbreak", "cfcontinue", "cfabort", "cfloop", "cfoutput", "cfscript",
                "cftry", "cfcatch", "cffinally", "cfthrow", "cfrethrow", "cfcontent", "cfflush",
                "cfapplication", "cfimage", "cfsilent", "cfinclude", "cfxml",
                "cffunction", "cfargument", "cfreturn", "cfquery",
                "cfcomponent", "cfproperty", "cfobject", "cfinvoke", "cfinvokeargument",
                "cfheader", "cflocation", "cfhttp", "cfhttpparam", "cftransaction",
                "cfqueryparam", "cfinsert", "cfupdate", "cfdbinfo",
                "cfstoredproc", "cfprocparam", "cfprocresult",
                "cfexit", "cferror", "cfcache",
                "cfimport", "cfmodule", "cfassociate",
                "cflog", "cftimer", "cftrace",
                "cfdirectory", "cffile", "cfzip", "cfzipparam",
                "cfparam", "cfobjectcache",
                "cfcookie", "cfhtmlhead", "cfprocessingdirective", "cfsavecontent", "cfsetting",
                "cfexecute", "cffeed", "cfwddx",
                "cflogin", "cfloginuser", "cflogout"
            };

            static const std::unordered_set<std::string> standard_tags = {
                "cfabort", "cfajaximport", "cfajaxproxy", "cfapplet", "cfargument", "cfassociate", "cfauthenticate",
                "cfbreak", "cfcache", "cfcalendar", "cfcase", "cfcatch", "cfchart", "cfchartdata", "cfchartseries", "cfchartset",
                "cfclient", "cfclientsettings", "cfcol", "cfcollection", "cfcomponent", "cfcontent", "cfcontinue", "cfcookie",
                "cfdbinfo", "cfdefaultcase", "cfdocument", "cfdocumentitem", "cfdocumentsection", "cfdump",
                "cfelse", "cfelseif", "cferror", "cfexchangecalendar", "cfexchangeconnection", "cfexchangecontact",
                "cfexchangeconversation", "cfexchangefilter", "cfexchangefolder", "cfexchangemail", "cfexchangetask",
                "cfexecute", "cfexit", "cffeed", "cffileupload", "cffinally", "cfflush", "cfform", "cfformgroup",
                "cfformitem", "cfftp", "cffunction", "cfgraph", "cfgraphdata", "cfgrid", "cfgridcolumn", "cfgridrow",
                "cfgridupdate", "cfheader", "cfhtmlhead", "cfhtmltopdf", "cfhtmltopdfitem", "cfhttp", "cfhttpparam", "cfif",
                "cfimage", "cfimap", "cfimapfilter", "cfimpersonate", "cfimport", "cfinclude", "cfindex", "cfinput",
                "cfinsert", "cfinterface", "cfinvoke", "cfinvokeargument", "cfjava", "cflayout", "cflayoutarea", "cfldap",
                "cflocation", "cflock", "cflog", "cflogin", "cfloginuser", "cflogout", "cfloop", "cfmail", "cfmailparam",
                "cfmailpart", "cfmap", "cfmapitem", "cfmediaplayer", "cfmenu", "cfmenuitem", "cfmessagebox", "cfmodule",
                "cfntauthenticate", "cfoauth", "cfobject", "cfobjectcache", "cfoutput", "cfparam", "cfpdf", "cfpdfform",
                "cfpdfformparam", "cfpdfparam", "cfpdfsubform", "cfpod", "cfpop", "cfpresentation", "cfpresentationslide",
                "cfpresenter", "cfprint", "cfprocessingdirective", "cfprocparam", "cfprocresult", "cfprogressbar", "cfproperty",
                "cfquery", "cfqueryparam", "cfregistry", "cfreport", "cfreportparam", "cfrethrow", "cfreturn", "cfsavecontent",
                "cfschedule", "cfscript", "cfsearch", "cfselect", "cfservlet", "cfservletparam", "cfset", "cfsetting",
                "cfsharepoint", "cfsilent", "cfslider", "cfspreadsheet", "cfsprydataset", "cfstoredproc", "cfswitch",
                "cftable", "cftextarea", "cftextinput", "cfthread", "cfthrow", "cftimer", "cftooltip", "cftrace", "cftransaction",
                "cftree", "cftreeitem", "cftry", "cfupdate", "cfwddx", "cfwebsocket", "cfwindow", "cfxml"
            };

            if (supported_tags.find(tagName) == supported_tags.end()) {
                if (standard_tags.find(tagName) != standard_tags.end()) {
                    throw webstrada::exception(("Tag " + tagName + " is not implemented").c_str());
                } else {
                    throw webstrada::exception(("Unknown CFML tag: " + tagName).c_str());
                }
            }
        }

        // 1. Output any raw text before this token (whitespace management
        //    applies to whitespace-only regions between CFML constructs; a
        //    CFML comment keeps the pending region alive).
        if (token.position > pos) {
            WsRight right = (token.token_id == TextParser_cfml_Comment) ? WsRight::Comment : WsRight::Tag;
            ws.feed(module, builder, out, cfm_text + pos, token.position - pos, right);
        }
        pos = token.position + token.len;

        switch (token.token_id) {
        case TextParser_cfml_Comment:
            break;

        case TextParser_cfml_StartTag: {
            string tokenName = string(cfm_text + token.position, token.len);
            // CFML tag names are case-insensitive; dispatch on a lowercased
            // copy while keeping `tokenName` (original case) for attribute
            // parsing, since attribute VALUES are case-sensitive.
            string tagNameLow = tokenName;
            tagNameLow.toLower();
            if (tagNameLow.startWith("<cfset") && !tagNameLow.startWith("<cfsetting")) {
                bool foundExpr = false;
                for (auto &child : token.children) {
                    if (child.token_id == TextParser_cfml_Expression) {
                        auto processedChildren = mergeObjectMembers(child.children);
                        TextParserTokenItem processedExpr = child;
                        processedExpr.children = processedChildren;

                        CfmlExpressionToClang(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, processedExpr, cfm_text);
                        foundExpr = true;
                        break;
                    }
                }
                if (!foundExpr) {
                    throw webstrada::exception("cfset", "Missing expression");
                }

            } else if (tagNameLow.startWith("<cfdump")) {
                auto dumpAttrs = parse_attributes(tokenName);
                if (!dumpAttrs.count("var")) {
                    throw webstrada::exception("cfdump", "Missing var parameter");
                }
                // Attribute order of cf_writedump:
                // var, output, format, abort, label, metainfo, top, show, hide, keys, expand, showUDFs
                const char *attrOrder[] = {"var", "output", "format", "abort", "label",
                                           "metainfo", "top", "show", "hide", "keys", "expand", "showudfs"};
                std::vector<llvm::Value*> dumpArgs;
                for (int ai = 0; ai < 12; ai++) {
                    auto it = dumpAttrs.find(attrOrder[ai]);
                    if (it == dumpAttrs.end()) {
                        dumpArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                        continue;
                    }
                    string av = it->second.trimmed();
                    if ((av.first() == '"' && av.at(av.length()-1) == '"') ||
                        (av.first() == '\'' && av.at(av.length()-1) == '\''))
                        av = av.mid(1, av.length() - 2).trimmed();
                    bool isExpr = av.length() >= 2 && av.first() == '#' && av.at(av.length()-1) == '#';
                    if (isExpr) av = av.mid(1, av.length() - 2).trimmed();

                    if (ai == 0 && isExpr) {
                        // var="#name#" → resolve the variable (or a scope)
                        string upperName = av;
                        upperName.toUpper();
                        llvm::Value *scopePtr = nullptr;
                        if (upperName.equals("VARIABLES"))       scopePtr = variables;
                        else if (upperName.equals("CGI"))        scopePtr = cgi;
                        else if (upperName.equals("URL"))        scopePtr = url;
                        else if (upperName.equals("FORM"))       scopePtr = form;
                        else if (upperName.equals("COOKIE"))     scopePtr = cookie;
                        else if (upperName.equals("SERVER"))     scopePtr = server;
                        else if (upperName.equals("APPLICATION")) scopePtr = application;
                        else if (upperName.equals("SESSION"))    scopePtr = session;
                        if (scopePtr) {
                            dumpArgs.push_back(scopePtr);
                        } else {
                            // General expression: compile it like any CFML
                            // expression so member chains (arguments.obj,
                            // s.key, arr[1].k) and calls resolve correctly.
                            // Previously only a single VARIABLES key worked
                            // (cfgetvar(variables, "arguments.obj") could
                            // never find dotted names).
                            llvm::Value *varPtr = CompileStringExpression(module, builder, mainfunc,
                                cgi, server, cookie, application, session, url, form, variables,
                                av, cfm_text);
                            dumpArgs.push_back(varPtr);
                        }
                    } else {
                        // Everything else is treated as a literal string, matching
                        // CF (e.g. var="scalar" dumps the string "scalar").
                        auto *fStr = module->getFunction("cfvariant_create_string");
                        if (!fStr) {
                            fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                                llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                        }
                        auto strGlobal = builder.CreateGlobalString(llvm::StringRef(av.constData(), av.length()), "", 0, module, true);
                        dumpArgs.push_back(emitCall(builder, fStr, {strGlobal}));
                    }
                }
                auto *fWritedump = module->getFunction("cf_writedump");
                if (!fWritedump) {
                    std::vector<llvm::Type*> params(12, builder.getPtrTy());
                    fWritedump = llvm::Function::Create(
                        llvm::FunctionType::get(builder.getPtrTy(), params, false),
                        llvm::Function::InternalLinkage, "cf_writedump", module);
                }
                auto *dumpResult = emitCall(builder, fWritedump, dumpArgs);
                auto *fEmit = module->getFunction("cf_emit_writedump");
                if (!fEmit) {
                    fEmit = llvm::Function::Create(
                        llvm::FunctionType::get(builder.getVoidTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                        llvm::Function::InternalLinkage, "cf_emit_writedump", module);
                }
                emitCall(builder, fEmit, {out, dumpResult});

            } else if (tagNameLow.startWith("<cfif")) {
                TextParserTokenItem ifExprToken;
                for (auto &child : token.children) {
                    if (child.token_id == TextParser_cfml_Expression) {
                        ifExprToken = child;
                        break;
                    }
                }
                if (ifExprToken.len == 0 && !token.children.empty()) {
                    // Single-token conditions (e.g. <cfif x>) arrive without an
                    // Expression child; use the tag children as the expression.
                    ifExprToken.children = token.children;
                }

                auto thenBB = llvm::BasicBlock::Create(context, "if.then", mainfunc);
                auto elseBB = llvm::BasicBlock::Create(context, "if.else", mainfunc);
                auto endBB = llvm::BasicBlock::Create(context, "if.end", mainfunc);

                auto *isTruthyFunc = module->getFunction("cfvariant_is_truthy");
                if (!isTruthyFunc) isTruthyFunc = llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_is_truthy", module);

                auto *condVal = CfmlExpressionToClang(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, ifExprToken, cfm_text);
                auto condResult = emitCall(builder, isTruthyFunc, {condVal});
                auto condBool = builder.CreateICmpNE(condResult, builder.getInt32(0));
                builder.CreateCondBr(condBool, thenBB, elseBB);

                struct BranchInfo {
                    TextParserTokenItem expressionToken;
                    string type;
                    std::vector<TextParserTokenItem> bodyTokens;
                    size_t bodyStartPos;
                    size_t bodyEndPos;
                };
                std::vector<BranchInfo> branches;
                branches.push_back({ifExprToken, "then", {}, token.position + token.len, 0});

                size_t scanIdx = index + 1;
                int nestedIfCount = 0;
                bool foundEnd = false;

                while (scanIdx < tokens.size()) {
                    const auto &tok = tokens[scanIdx];
                    if (tok.token_id == TextParser_cfml_StartTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("<cfif")) {
                            nestedIfCount++;
                        } else if (tn.startWith("<cfelseif") && nestedIfCount == 0) {
                            branches.back().bodyEndPos = tok.position;
                            TextParserTokenItem elseifExprToken;
                            for (auto &child : tok.children) {
                                if (child.token_id == TextParser_cfml_Expression) {
                                    elseifExprToken = child;
                                    break;
                                }
                            }
                            if (elseifExprToken.len == 0 && !tok.children.empty()) {
                                elseifExprToken.children = tok.children;
                            }
                            branches.push_back({elseifExprToken, "elseif", {}, tok.position + tok.len, 0});
                            scanIdx++;
                            continue;
                        } else if (tn.startWith("<cfelse") && nestedIfCount == 0) {
                            branches.back().bodyEndPos = tok.position;
                            branches.push_back({{}, "else", {}, tok.position + tok.len, 0});
                            scanIdx++;
                            continue;
                        }
                    } else if (tok.token_id == TextParser_cfml_EndTag) {
                        string en(cfm_text + tok.position, tok.len);
                        en.toLower();
                        if (en.startWith("</cfif")) {
                            if (nestedIfCount > 0) {
                                nestedIfCount--;
                            } else {
                                branches.back().bodyEndPos = tok.position;
                                foundEnd = true;
                                scanIdx++;
                                break;
                            }
                        }
                    }
                    branches.back().bodyTokens.push_back(tok);
                    scanIdx++;
                }

                index = scanIdx - 1;

                // Compile branches
                builder.SetInsertPoint(thenBB);
                {
                    WhitespaceState wsBranch(ws.enabled, ws.flag);
                    wsBranch.markTag(false, false); // left neighbour is <cfif>
                    size_t branchPos = branches[0].bodyStartPos;
                    size_t bidx = 0;
                    compile_token_list(branches[0].bodyTokens, bidx, branchPos, context, module, builder, mainfunc,
                                       out, wsBranch, cgi, server, cookie, application, session, url, form, variables,
                                       cfm_text, branches[0].bodyEndPos, loopStack);
                    if (branchPos < branches[0].bodyEndPos) {
                        wsBranch.feed(module, builder, out, cfm_text + branchPos, branches[0].bodyEndPos - branchPos, WsRight::Tag);
                    }
                    wsBranch.finish(module, builder, out, WsRight::Tag);
                    if (!builder.GetInsertBlock()->getTerminator()) {
                        builder.CreateBr(endBB);
                    }
                }

                builder.SetInsertPoint(elseBB);
                llvm::BasicBlock *currentElseBB = elseBB;
                for (size_t b = 1; b < branches.size(); b++) {
                    const auto &branch = branches[b];
                    if (branch.type.equals("elseif")) {
                        auto elseifThenBB = llvm::BasicBlock::Create(context, "if.elseif.then", mainfunc);
                        auto elseifNextBB = llvm::BasicBlock::Create(context, "if.elseif.next", mainfunc);

                        auto *elseifCondVal = CfmlExpressionToClang(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, branch.expressionToken, cfm_text);
                        auto elseifResult = emitCall(builder, isTruthyFunc, {elseifCondVal});
                        auto elseifBool = builder.CreateICmpNE(elseifResult, builder.getInt32(0));
                        builder.CreateCondBr(elseifBool, elseifThenBB, elseifNextBB);

                        builder.SetInsertPoint(elseifThenBB);
                        {
                            WhitespaceState wsElseif(ws.enabled, ws.flag);
                            wsElseif.markTag(false, false); // left neighbour is <cfelseif>
                            size_t branchPos = branch.bodyStartPos;
                            size_t bidx = 0;
                            compile_token_list(branch.bodyTokens, bidx, branchPos, context, module, builder, mainfunc,
                                               out, wsElseif, cgi, server, cookie, application, session, url, form, variables,
                                               cfm_text, branch.bodyEndPos, loopStack);
                            if (branchPos < branch.bodyEndPos) {
                                wsElseif.feed(module, builder, out, cfm_text + branchPos, branch.bodyEndPos - branchPos, WsRight::Tag);
                            }
                            wsElseif.finish(module, builder, out, WsRight::Tag);
                            if (!builder.GetInsertBlock()->getTerminator()) {
                                builder.CreateBr(endBB);
                            }
                        }

                        builder.SetInsertPoint(elseifNextBB);
                        currentElseBB = elseifNextBB;
                    } else if (branch.type.equals("else")) {
                        WhitespaceState wsElse(ws.enabled, ws.flag);
                        wsElse.markTag(false, false); // left neighbour is <cfelse>
                        size_t branchPos = branch.bodyStartPos;
                        size_t bidx = 0;
                        compile_token_list(branch.bodyTokens, bidx, branchPos, context, module, builder, mainfunc,
                                           out, wsElse, cgi, server, cookie, application, session, url, form, variables,
                                           cfm_text, branch.bodyEndPos, loopStack);
                        if (branchPos < branch.bodyEndPos) {
                            wsElse.feed(module, builder, out, cfm_text + branchPos, branch.bodyEndPos - branchPos, WsRight::Tag);
                        }
                        wsElse.finish(module, builder, out, WsRight::Tag);
                        if (!builder.GetInsertBlock()->getTerminator()) {
                            builder.CreateBr(endBB);
                        }
                    }
                }

                if (!builder.GetInsertBlock()->getTerminator()) {
                    builder.CreateBr(endBB);
                }

                builder.SetInsertPoint(endBB);
                pos = token.position + token.len;
                if (foundEnd && index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }

            } else if (tagNameLow.startWith("<cfswitch")) {
                auto switchAttrs = parse_attributes(tokenName);
                string switchExpr;
                if (switchAttrs.count("expression")) {
                    switchExpr = switchAttrs["expression"].trimmed();
                    if ((switchExpr.first() == '"' && switchExpr.at(switchExpr.length()-1) == '"') ||
                        (switchExpr.first() == '\'' && switchExpr.at(switchExpr.length()-1) == '\''))
                        switchExpr = switchExpr.mid(1, switchExpr.length() - 2).trimmed();
                    if (switchExpr.length() >= 2 && switchExpr.first() == '#' && switchExpr.at(switchExpr.length()-1) == '#')
                        switchExpr = switchExpr.mid(1, switchExpr.length() - 2).trimmed();
                } else {
                    throw webstrada::exception("cfswitch", "Missing expression attribute");
                }

                auto *switchVal = CompileStringExpression(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, switchExpr, cfm_text);

                struct CaseBranchInfo {
                    string values;
                    string delimiters;
                    std::vector<TextParserTokenItem> bodyTokens;
                    size_t bodyStartPos;
                    size_t bodyEndPos;
                    bool isDefault;
                };
                std::vector<CaseBranchInfo> cases;

                size_t scanIdx = index + 1;
                int nestedSwitchCount = 0;
                bool insideCase = false;

                while (scanIdx < tokens.size()) {
                    const auto &tok = tokens[scanIdx];
                    if (tok.token_id == TextParser_cfml_StartTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        string tnLow = tn;
                        tnLow.toLower();
                        if (tnLow.startWith("<cfswitch")) {
                            nestedSwitchCount++;
                        } else if (tnLow.startWith("<cfcase") && nestedSwitchCount == 0) {
                            auto caseAttrs = parse_attributes(tn);
                            string vals = caseAttrs.count("value") ? caseAttrs["value"] : "";
                            string dels = caseAttrs.count("delimiters") ? caseAttrs["delimiters"] : ",";
                            cases.push_back({vals, dels, {}, tok.position + tok.len, 0, false});
                            insideCase = true;
                            scanIdx++;
                            continue;
                        } else if (tnLow.startWith("<cfdefaultcase") && nestedSwitchCount == 0) {
                            cases.push_back({"", "", {}, tok.position + tok.len, 0, true});
                            insideCase = true;
                            scanIdx++;
                            continue;
                        }
                    } else if (tok.token_id == TextParser_cfml_EndTag) {
                        string en(cfm_text + tok.position, tok.len);
                        en.toLower();
                        if (en.startWith("</cfcase") && nestedSwitchCount == 0) {
                            if (!cases.empty() && !cases.back().isDefault) {
                                cases.back().bodyEndPos = tok.position;
                            }
                            insideCase = false;
                            scanIdx++;
                            continue;
                        } else if (en.startWith("</cfdefaultcase") && nestedSwitchCount == 0) {
                            if (!cases.empty() && cases.back().isDefault) {
                                cases.back().bodyEndPos = tok.position;
                            }
                            insideCase = false;
                            scanIdx++;
                            continue;
                        } else if (en.startWith("</cfswitch")) {
                            if (nestedSwitchCount > 0) {
                                nestedSwitchCount--;
                            } else {
                                scanIdx++;
                                break;
                            }
                        }
                    }

                    if (insideCase && !cases.empty()) {
                        cases.back().bodyTokens.push_back(tok);
                    }
                    scanIdx++;
                }

                index = scanIdx - 1;

                auto *endBB = llvm::BasicBlock::Create(context, "switch.end", mainfunc);
                std::vector<llvm::BasicBlock*> bodyBBs;
                llvm::BasicBlock *defaultBB = nullptr;

                for (size_t cIdx = 0; cIdx < cases.size(); cIdx++) {
                    if (cases[cIdx].isDefault) {
                        defaultBB = llvm::BasicBlock::Create(context, "switch.default", mainfunc);
                    } else {
                        bodyBBs.push_back(llvm::BasicBlock::Create(context, ("switch.body" + std::to_string(cIdx)).c_str(), mainfunc));
                    }
                }

                llvm::BasicBlock *prevBB = builder.GetInsertBlock();
                size_t bodyBBIdx = 0;

                auto *compareFunc = module->getFunction("cfvariant_compare");
                if (!compareFunc) compareFunc = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_compare", module);
                auto *isTruthyFunc = module->getFunction("cfvariant_is_truthy");
                if (!isTruthyFunc) isTruthyFunc = llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_is_truthy", module);

                for (size_t cIdx = 0; cIdx < cases.size(); cIdx++) {
                    const auto &cBranch = cases[cIdx];
                    if (cBranch.isDefault) continue;

                    string vals = cBranch.values;
                    string dels = cBranch.delimiters;
                    std::vector<webstrada::string> valList;
                    {
                        size_t s = 0;
                        for (size_t j = 0; j < vals.length(); j++) {
                            if (dels.indexOf(vals.at(j)) >= 0) {
                                string v = vals.mid(s, j - s).trimmed();
                                if (!v.isEmpty()) valList.push_back(v);
                                s = j + 1;
                            }
                        }
                        string lv = vals.mid(s, vals.length() - s);
                        string lvt = lv.trimmed();
                        if (!lvt.isEmpty()) valList.push_back(lvt);
                    }

                    auto *checkBB = llvm::BasicBlock::Create(context, ("switch.check" + std::to_string(cIdx)).c_str(), mainfunc);
                    builder.SetInsertPoint(prevBB);
                    builder.CreateBr(checkBB);
                    builder.SetInsertPoint(checkBB);

                    llvm::Value *combined = nullptr;
                    for (auto &v : valList) {
                        char *ep = nullptr;
                        strtol(v.constData(), &ep, 10);
                        bool isNum = (*ep == '\0');
                        bool isBool = v.equals("true") || v.equals("false") || v.equals("yes") || v.equals("no");
                        string cmpExpr = isNum || isBool ? v : "\"" + v + "\"";

                        auto *caseVal = CompileStringExpression(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, cmpExpr, cfm_text);
                        auto *cmpResult = emitCall(builder, compareFunc, {switchVal, caseVal, builder.CreateGlobalString("EQ", "", 0, module, true)});
                        auto *isCmpTrue = emitCall(builder, isTruthyFunc, {cmpResult});
                        auto *mb = builder.CreateICmpNE(isCmpTrue, builder.getInt32(0));
                        combined = combined ? builder.CreateOr(combined, mb) : mb;
                    }

                    auto *targetBodyBB = bodyBBs[bodyBBIdx++];
                    llvm::BasicBlock *nextBB = nullptr;
                    if (cIdx + 1 < cases.size()) {
                        nextBB = llvm::BasicBlock::Create(context, ("switch.check" + std::to_string(cIdx + 1)).c_str(), mainfunc);
                    } else {
                        nextBB = defaultBB ? defaultBB : endBB;
                    }

                    if (combined) {
                        builder.CreateCondBr(combined, targetBodyBB, nextBB);
                    } else {
                        builder.CreateBr(nextBB);
                    }
                    prevBB = nextBB;
                }

                if (prevBB && prevBB != endBB && prevBB != defaultBB) {
                    builder.SetInsertPoint(prevBB);
                    builder.CreateBr(defaultBB ? defaultBB : endBB);
                }

                // Compile case bodies
                bodyBBIdx = 0;
                for (size_t cIdx = 0; cIdx < cases.size(); cIdx++) {
                    const auto &cBranch = cases[cIdx];
                    if (cBranch.isDefault) {
                        builder.SetInsertPoint(defaultBB);
                        WhitespaceState wsCase(ws.enabled, ws.flag);
                        wsCase.markTag(false, false); // left neighbour is <cfdefaultcase>
                        size_t casePos = cBranch.bodyStartPos;
                        size_t cbIdx = 0;
                        compile_token_list(cBranch.bodyTokens, cbIdx, casePos, context, module, builder, mainfunc,
                                           out, wsCase, cgi, server, cookie, application, session, url, form, variables,
                                           cfm_text, cBranch.bodyEndPos, loopStack);
                        if (casePos < cBranch.bodyEndPos) {
                            wsCase.feed(module, builder, out, cfm_text + casePos, cBranch.bodyEndPos - casePos, WsRight::Tag);
                        }
                        wsCase.finish(module, builder, out, WsRight::Tag);
                        if (!builder.GetInsertBlock()->getTerminator()) {
                            builder.CreateBr(endBB);
                        }
                    } else {
                        builder.SetInsertPoint(bodyBBs[bodyBBIdx++]);
                        WhitespaceState wsCase(ws.enabled, ws.flag);
                        wsCase.markTag(false, false); // left neighbour is <cfcase>
                        size_t casePos = cBranch.bodyStartPos;
                        size_t cbIdx = 0;
                        compile_token_list(cBranch.bodyTokens, cbIdx, casePos, context, module, builder, mainfunc,
                                           out, wsCase, cgi, server, cookie, application, session, url, form, variables,
                                           cfm_text, cBranch.bodyEndPos, loopStack);
                        if (casePos < cBranch.bodyEndPos) {
                            wsCase.feed(module, builder, out, cfm_text + casePos, cBranch.bodyEndPos - casePos, WsRight::Tag);
                        }
                        wsCase.finish(module, builder, out, WsRight::Tag);
                        if (!builder.GetInsertBlock()->getTerminator()) {
                            builder.CreateBr(endBB);
                        }
                    }
                }

                builder.SetInsertPoint(endBB);
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }

            } else if (tagNameLow.startWith("<cfbreak")) {
                if (!loopStack.empty()) {
                    builder.CreateBr(loopStack.back().endBB);
                    auto deadBB = llvm::BasicBlock::Create(context, "cfbreak.cont", mainfunc);
                    builder.SetInsertPoint(deadBB);
                } else {
                    throw webstrada::exception("cfbreak", "cfbreak tags are only valid inside a cfloop block");
                }
            } else if (tagNameLow.startWith("<cfcontinue")) {
                if (!loopStack.empty()) {
                    builder.CreateBr(loopStack.back().incBB);
                    auto deadBB = llvm::BasicBlock::Create(context, "cfcontinue.cont", mainfunc);
                    builder.SetInsertPoint(deadBB);
                } else {
                    throw webstrada::exception("cfcontinue", "cfcontinue tags are only valid inside a cfloop block");
                }
            } else if (tagNameLow.startWith("<cfabort")) {
                llvm_CfAbort(module, builder);
                auto deadBB = llvm::BasicBlock::Create(context, "cfabort.cont", mainfunc);
                builder.SetInsertPoint(deadBB);
            } else if (tagNameLow.startWith("<cfexit")) {
                // <cfexit [method="..."]> — aborts the currently executing
                // template page (method exittag/exittemplate, the default;
                // case-insensitive), returns undefined from the enclosing
                // function body, or throws CF's catchable "loop can be used
                // only inside custom tags" error. Attribute validation is at
                // compile time (unknown attrs; invalid static method literals
                // — CF renders an error page even for a static value); a
                // dynamically evaluated method dispatches at runtime.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto tagAttrs = parseTagAttrs(attrParts, cfm_text);

                static const std::unordered_set<std::string> exitValidAttrs = {"method"};
                std::vector<std::string> unknownAttrs;
                for (const auto &a : tagAttrs) {
                    if (exitValidAttrs.find(lowercase(a.first)) == exitValidAttrs.end()) {
                        unknownAttrs.push_back(a.first);
                    }
                }
                if (!unknownAttrs.empty()) {
                    std::sort(unknownAttrs.begin(), unknownAttrs.end(),
                              [](const std::string &a, const std::string &b) { return lowercase(a) < lowercase(b); });
                    std::string list;
                    for (size_t i = 0; i < unknownAttrs.size(); i++) {
                        if (i) list += ',';
                        list += uppercase(unknownAttrs[i]);
                    }
                    throw webstrada::exception(("Attribute validation error for tag CFEXIT. It does not allow the attribute(s) " + list + ". The valid attribute(s) are METHOD.").c_str());
                }

                // The method value: nullptr when absent. StaticMethodClass is
                // -1 (absent/dynamic), 0 (exittag/exittemplate) or 1 (loop).
                const std::vector<TextParserTokenItem> *methodToks = nullptr;
                for (const auto &a : tagAttrs) {
                    if (lowercase(a.first) == "method") {
                        methodToks = &a.second;
                        break;
                    }
                }
                int staticMethodClass = -1;
                if (methodToks && methodToks->size() == 1 &&
                    ((*methodToks)[0].token_id == TextParser_cfml_DoubleString ||
                     (*methodToks)[0].token_id == TextParser_cfml_SingleString) &&
                    (*methodToks)[0].children.empty()) {
                    // Plain quoted literal without #...# interpolation: the
                    // method is known at compile time and an invalid value is
                    // a compile-time error (uncatchable, like CF).
                    string raw(cfm_text + (*methodToks)[0].position + 1, (*methodToks)[0].len - 2);
                    string rawLow = raw;
                    rawLow.toLower();
                    if (rawLow.equals("exittag") || rawLow.equals("exittemplate")) {
                        staticMethodClass = 0;
                    } else if (rawLow.equals("loop")) {
                        staticMethodClass = 1;
                    } else {
                        std::string shown = raw.isEmpty() ? "''" : std::string(raw.constData(), raw.length());
                        throw webstrada::exception(("Attribute validation error for CFEXIT. The value of the METHOD attribute, which is currently " + shown + ", must be one of the values: EXITTAG,LOOP,EXITTEMPLATE.").c_str());
                    }
                }

                auto *fExit = getOrCreateHelper(module, builder, "cf_exit", builder.getVoidTy(), {});
                auto *fExitLoop = getOrCreateHelper(module, builder, "cf_exit_loop", builder.getVoidTy(), {});
                auto *fExitInvalid = getOrCreateHelper(module, builder, "cf_exit_invalid", builder.getVoidTy(), {builder.getPtrTy()});
                auto *fClassify = getOrCreateHelper(module, builder, "cf_exit_classify", builder.getInt32Ty(), {builder.getPtrTy()});

                auto emitExitPath = [&](llvm::Value *methodVal) {
                    // methodVal != nullptr: runtime dispatch on the evaluated
                    // value (0 = exit, 1 = loop, else invalid method).
                    if (methodVal) {
                        llvm::Value *code = emitCall(builder, fClassify, {methodVal});
                        llvm::BasicBlock *exitBB = llvm::BasicBlock::Create(context, "cfexit.exit", mainfunc);
                        llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(context, "cfexit.loop", mainfunc);
                        llvm::BasicBlock *invalidBB = llvm::BasicBlock::Create(context, "cfexit.invalid", mainfunc);
                        auto *sw = builder.CreateSwitch(code, invalidBB, 2);
                        sw->addCase(builder.getInt32(0), exitBB);
                        sw->addCase(builder.getInt32(1), loopBB);
                        builder.SetInsertPoint(loopBB);
                        emitCall(builder, fExitLoop, {});
                        builder.CreateUnreachable();
                        builder.SetInsertPoint(invalidBB);
                        emitCall(builder, fExitInvalid, {methodVal});
                        builder.CreateUnreachable();
                        builder.SetInsertPoint(exitBB);
                    }
                    if (g_returnCtx) {
                        // Inside a function body <cfexit> returns undefined,
                        // exactly like a bare <cfreturn> (verified on CF).
                        builder.CreateBr(g_returnCtx->exitBB);
                    } else {
                        emitCall(builder, fExit, {});
                        builder.CreateUnreachable();
                    }
                    auto deadBB = llvm::BasicBlock::Create(context, "cfexit.cont", mainfunc);
                    builder.SetInsertPoint(deadBB);
                };

                if (staticMethodClass == 1) {
                    // method="loop" is always a runtime error (catchable by a
                    // CFML catch), even for a static literal.
                    emitCall(builder, fExitLoop, {});
                    builder.CreateUnreachable();
                    auto deadBB = llvm::BasicBlock::Create(context, "cfexit.cont", mainfunc);
                    builder.SetInsertPoint(deadBB);
                } else if (staticMethodClass == 0) {
                    emitExitPath(nullptr);
                } else {
                    // Absent or dynamically evaluated method: evaluate the
                    // value and dispatch at runtime.
                    llvm::Value *methodVal = llvm::ConstantPointerNull::get(builder.getPtrTy());
                    if (methodToks) {
                        if (methodToks->size() == 1 &&
                            ((*methodToks)[0].token_id == TextParser_cfml_DoubleString ||
                             (*methodToks)[0].token_id == TextParser_cfml_SingleString)) {
                            auto node = std::make_unique<ExprAST>();
                            node->type = ExprAST::LiteralString;
                            node->token = (*methodToks)[0];
                            methodVal = CompileExprAST(module, builder, mainfunc, node, cgi, server, cookie,
                                                       application, session, url, form, variables, cfm_text);
                        } else {
                            auto ast = parseTokensToAST(*methodToks, cfm_text);
                            methodVal = CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                                       application, session, url, form, variables, cfm_text);
                        }
                    }
                    emitExitPath(methodVal);
                }
            } else if (tagNameLow.startWith("<cfflush")) {
                // Flush buffered output to the web engine (no-op when empty).
                llvm_CfFlush(module, builder, out);
            } else if (tagNameLow.startWith("<cfcontent")) {
                // <cfcontent> sets the response MIME type/charset (type=...;
                // charset=...), optionally discards prior output (reset, default
                // yes) or replaces the page output with a file's / variable's
                // contents. The runtime enforces that the type/charset can only
                // change before the first byte is written to the web engine.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *typeVal = nullPtr, *resetVal = nullPtr, *fileVal = nullPtr,
                            *varVal = nullPtr, *delVal = nullPtr;
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_DoubleString ||
                         valToks[0].token_id == TextParser_cfml_SingleString)) {
                        // Quoted literal (or "#expr#" interpolation): build a
                        // LiteralString AST so CompileExprAST handles both.
                        auto node = std::make_unique<ExprAST>();
                        node->type = ExprAST::LiteralString;
                        node->token = valToks[0];
                        return CompileExprAST(module, builder, mainfunc, node, cgi, server, cookie,
                                              application, session, url, form, variables, cfm_text);
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    aname.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size() && isOperatorToken((*attrParts)[vi].token_id)) vi++;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    llvm::Value *val = compileValue(valToks);
                    if (aname.equals("type")) typeVal = val;
                    else if (aname.equals("reset")) resetVal = val;
                    else if (aname.equals("file")) fileVal = val;
                    else if (aname.equals("variable")) varVal = val;
                    else if (aname.equals("deletefile")) delVal = val;
                    ai = vi;
                }
                llvm_CfContent(module, builder, out, typeVal, resetVal, fileVal, varVal, delVal);
            } else if (tagNameLow.startWith("<cfheader")) {
                // <cfheader> queues a custom HTTP response header (name/value,
                // value re-encoded with charset when given, name=content-type
                // routed through the response MIME type) and/or sets the HTTP
                // status code (statuscode). Attribute validation mirrors CF:
                // unknown attributes and invalid name/statuscode combinations
                // are compile-time errors with CF's exact messages.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto tagAttrs = parseTagAttrs(attrParts, cfm_text);

                static const std::unordered_set<std::string> headerValidAttrs =
                    {"name", "value", "charset", "statuscode", "statustext"};
                std::vector<std::string> unknownAttrs;
                bool hasStatustext = false;
                for (const auto &a : tagAttrs) {
                    std::string aLow = lowercase(a.first);
                    if (aLow == "statustext") {
                        hasStatustext = true;
                    }
                    if (headerValidAttrs.find(aLow) == headerValidAttrs.end()) {
                        unknownAttrs.push_back(a.first);
                    }
                }
                if (!unknownAttrs.empty()) {
                    std::sort(unknownAttrs.begin(), unknownAttrs.end(),
                              [](const std::string &a, const std::string &b) { return lowercase(a) < lowercase(b); });
                    std::string list;
                    for (size_t i = 0; i < unknownAttrs.size(); i++) {
                        if (i) list += ',';
                        list += uppercase(unknownAttrs[i]);
                    }
                    throw webstrada::exception(("Attribute validation error for tag CFHEADER. It does not allow the attribute(s) " + list + ". The valid attribute(s) are CHARSET,NAME,STATUSCODE,VALUE.").c_str());
                }

                if (hasStatustext) {
                    int line = lineOfOffset(cfm_text, token.position);
                    size_t line_idx = g_textparser ? textparser_get_line_number_at_position(g_textparser, token.position) : 0;
                    size_t line_start = g_textparser ? textparser_get_line_start_position(g_textparser, line_idx) : 0;
                    size_t col = (token.position >= line_start) ? (token.position - line_start + 1) : 1;
                    std::string path = module ? module->getName().str() : "";
                    fprintf(stderr, "[WebStrada] Warning: tag <cfheader> attribute 'statustext' is deprecated and ignored (%s:%d:%zu)\n",
                            path.c_str(), line, col);
                }

                // Valid combinations (CF's HeaderTag bean): name (with optional
                // value/charset) OR statuscode (optionally with deprecated statustext).
                // Sorted-lowercase combo string appears in the error message like CF.
                std::vector<std::string> combo;
                for (const auto &a : tagAttrs) {
                    std::string aLow = lowercase(a.first);
                    if (aLow != "statustext") combo.push_back(aLow);
                }
                std::sort(combo.begin(), combo.end());
                bool validCombo = (combo.size() == 1 && combo[0] == "statuscode");
                if (!validCombo) {
                    bool hasName = false, allKnown = true;
                    for (const auto &n : combo) {
                        if (n == "name") hasName = true;
                        else if (n != "value" && n != "charset") allKnown = false;
                    }
                    validCombo = hasName && allKnown;
                }
                if (!validCombo) {
                    std::string comboStr;
                    for (size_t i = 0; i < combo.size(); i++) {
                        if (i) comboStr += ',';
                        comboStr += combo[i];
                    }
                    throw webstrada::exception(("Attribute validation error for tag CFHEADER. It has an invalid attribute combination: '" + comboStr + "'. Possible combinations are: Required attributes: 'name'. Optional attributes: 'charset,value'. Required attributes: 'statuscode'. Optional attributes: None.").c_str());
                }

                llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *nameVal = nullPtr, *valueVal = nullPtr, *charsetVal = nullPtr, *statusVal = nullPtr;
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_DoubleString ||
                         valToks[0].token_id == TextParser_cfml_SingleString)) {
                        auto node = std::make_unique<ExprAST>();
                        node->type = ExprAST::LiteralString;
                        node->token = valToks[0];
                        return CompileExprAST(module, builder, mainfunc, node, cgi, server, cookie,
                                              application, session, url, form, variables, cfm_text);
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                for (const auto &a : tagAttrs) {
                    llvm::Value *val = compileValue(a.second);
                    std::string low = lowercase(a.first);
                    if (low == "name") nameVal = val;
                    else if (low == "value") valueVal = val;
                    else if (low == "charset") charsetVal = val;
                    else if (low == "statuscode") statusVal = val;
                }
                auto addHeader = get_response_add_header_func(module, builder);
                emitCall(builder, addHeader, {nameVal, valueVal, charsetVal, statusVal});
            } else if (tagNameLow.startWith("<cflocation")) {
                // <cflocation> sends an HTTP redirect (Location header + 3xx
                // status) and stops the current page. url is required and the
                // statuscode range is validated (CF messages); the runtime
                // helper aborts the page like <cfabort>.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto tagAttrs = parseTagAttrs(attrParts, cfm_text);

                static const std::unordered_set<std::string> locValidAttrs =
                    {"url", "addtoken", "statuscode"};
                std::vector<std::string> unknownAttrs;
                for (const auto &a : tagAttrs) {
                    if (locValidAttrs.find(lowercase(a.first)) == locValidAttrs.end()) {
                        unknownAttrs.push_back(a.first);
                    }
                }
                if (!unknownAttrs.empty()) {
                    std::sort(unknownAttrs.begin(), unknownAttrs.end(),
                              [](const std::string &a, const std::string &b) { return lowercase(a) < lowercase(b); });
                    std::string list;
                    for (size_t i = 0; i < unknownAttrs.size(); i++) {
                        if (i) list += ',';
                        list += uppercase(unknownAttrs[i]);
                    }
                    throw webstrada::exception(("Attribute validation error for tag CFLOCATION. It does not allow the attribute(s) " + list + ". The valid attribute(s) are ADDTOKEN,STATUSCODE,URL.").c_str());
                }
                bool hasUrl = false;
                for (const auto &a : tagAttrs) {
                    if (lowercase(a.first) == "url") hasUrl = true;
                }
                if (!hasUrl) {
                    throw webstrada::exception("Attribute validation error for tag CFLOCATION. It requires the attribute(s): URL.");
                }
                llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *urlVal = nullPtr, *addTokenVal = nullPtr, *statusVal = nullPtr;
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_DoubleString ||
                         valToks[0].token_id == TextParser_cfml_SingleString)) {
                        auto node = std::make_unique<ExprAST>();
                        node->type = ExprAST::LiteralString;
                        node->token = valToks[0];
                        return CompileExprAST(module, builder, mainfunc, node, cgi, server, cookie,
                                              application, session, url, form, variables, cfm_text);
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                for (const auto &a : tagAttrs) {
                    llvm::Value *val = compileValue(a.second);
                    std::string low = lowercase(a.first);
                    if (low == "url") urlVal = val;
                    else if (low == "addtoken") addTokenVal = val;
                    else if (low == "statuscode") statusVal = val;
                }
                auto redirect = get_response_redirect_func(module, builder);
                emitCall(builder, redirect, {urlVal, addTokenVal, statusVal});
                // The redirect always aborts the page; the following code is
                // unreachable (like <cfabort>).
                builder.CreateUnreachable();
                auto deadBB = llvm::BasicBlock::Create(context, "cflocation.cont", mainfunc);
                builder.SetInsertPoint(deadBB);
            } else if (tagNameLow.startWith("<cftry")) {
                index = compile_tag_try_statement(tokens, index, context, module, builder, mainfunc,
                                                  out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                  cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfsilent")) {
                index = compile_tag_silent_statement(tokens, index, context, module, builder, mainfunc,
                                                     out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                     cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfcache")) {
                index = compile_tag_cache_statement(tokens, index, context, module, builder, mainfunc,
                                                    out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                    cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfxml")) {
                index = compile_tag_xml_statement(tokens, index, context, module, builder, mainfunc,
                                                  out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                  cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfhttpparam")) {
                // <cfhttpparam type=".." name=".." value=".." [file] [encoded]
                // [mimetype]> inside a <cfhttp> body: append a parameter to the
                // current request builder.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                auto *fParam = getOrCreateHelper(module, builder, "cf_http_param", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy()});
                auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *typeVal = nullPtr, *nameVal = nullPtr, *valueVal = nullPtr,
                            *fileVal = nullPtr, *encodedVal = nullPtr, *mimeVal = nullPtr;
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    llvm::Value *val = compileValue(valToks);
                    if (anameLow.equals("type")) typeVal = val;
                    else if (anameLow.equals("name")) nameVal = val;
                    else if (anameLow.equals("value")) valueVal = val;
                    else if (anameLow.equals("file")) fileVal = val;
                    else if (anameLow.equals("encoded")) encodedVal = val;
                    else if (anameLow.equals("mimetype")) mimeVal = val;
                    ai = vi;
                }
                emitCall(builder, fParam, {typeVal, nameVal, valueVal, fileVal, encodedVal, mimeVal});
            } else if (tagNameLow.startWith("<cfprocparam")) {
                // <cfprocparam type=".." variable=".." value=".." cfsqltype=".."
                // maxlength=".." scale=".." null=".." dbvarname=".."> inside a
                // <cfstoredproc> body: append a parameter to the call context.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                auto *fParam = getOrCreateHelper(module, builder, "cf_proc_param", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *typeVal = nullPtr, *variableVal = nullPtr, *valueVal = nullPtr,
                            *sqltypeVal = nullPtr, *maxlenVal = nullPtr, *scaleVal = nullPtr,
                            *nullVal = nullPtr, *dbvarVal = nullPtr;
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    llvm::Value *val = compileValue(valToks);
                    if (anameLow.equals("type")) typeVal = val;
                    else if (anameLow.equals("variable")) variableVal = val;
                    else if (anameLow.equals("value")) valueVal = val;
                    else if (anameLow.equals("cfsqltype")) sqltypeVal = val;
                    else if (anameLow.equals("maxlength")) maxlenVal = val;
                    else if (anameLow.equals("scale")) scaleVal = val;
                    else if (anameLow.equals("null")) nullVal = val;
                    else if (anameLow.equals("dbvarname")) dbvarVal = val;
                    ai = vi;
                }
                emitCall(builder, fParam, {typeVal, variableVal, valueVal, sqltypeVal,
                                           maxlenVal, scaleVal, nullVal, dbvarVal});
            } else if (tagNameLow.startWith("<cfprocresult")) {
                // <cfprocresult name=".." resultset=".." maxrows=".."> inside a
                // <cfstoredproc> body: append a result-set binding.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                auto *fRes = getOrCreateHelper(module, builder, "cf_proc_result", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *nameVal = nullPtr, *resultsetVal = nullPtr, *maxrowsVal = nullPtr;
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    llvm::Value *val = compileValue(valToks);
                    if (anameLow.equals("name")) nameVal = val;
                    else if (anameLow.equals("resultset")) resultsetVal = val;
                    else if (anameLow.equals("maxrows")) maxrowsVal = val;
                    ai = vi;
                }
                emitCall(builder, fRes, {nameVal, resultsetVal, maxrowsVal});
            } else if (tagNameLow.startWith("<cfhttp") && !tagNameLow.startWith("<cfhttpparam")) {
                index = compile_tag_http_statement(tokens, index, context, module, builder, mainfunc,
                                                   out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                   cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cftransaction")) {
                index = compile_tag_transaction_statement(tokens, index, context, module, builder, mainfunc,
                                                          out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                          cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfstoredproc")) {
                index = compile_tag_storedproc_statement(tokens, index, context, module, builder, mainfunc,
                                                         out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                         cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cflog") && !tagNameLow.startWith("<cflogin") && !tagNameLow.startWith("<cflogout")) {
                // <cflog text=".." [type] [application] [file] [log]> writes a
                // message to a log file and renders nothing. The runtime is the
                // shared WriteLog helper (cf_writelog). Attribute validation is
                // compile-time like CF: unknown attributes →
                // "Attribute validation error for tag CFLOG. It does not allow
                // the attribute(s) X. The valid attribute(s) are
                // APPLICATION,FILE,LOG,TEXT,TYPE." (unknown attrs win over a
                // missing text); a missing `text` →
                // "Attribute validation error for tag CFLOG. It requires the
                // attribute(s): TEXT." The body is SKIPped like CF's LogTag
                // (doStartTag returns SKIP_BODY — verified: side effects in the
                // body do not happen), but it is still compiled into a dead
                // block so a body syntax error surfaces at compile time.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                static const std::unordered_set<std::string> logValidAttrs = {
                    "text", "type", "application", "file", "log"};
                std::vector<std::string> unknownAttrs;
                std::vector<std::pair<std::string, std::vector<TextParserTokenItem>>> logAttrs;
                bool hasText = false;
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id) &&
                                           isAttrNameToken(*attrParts, vi, cfm_text));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (logValidAttrs.find(anameLow.constData()) == logValidAttrs.end()) {
                        unknownAttrs.push_back(aname.constData());
                    } else {
                        if (anameLow.equals("text")) hasText = true;
                        logAttrs.push_back({anameLow.constData(), valToks});
                    }
                    ai = vi;
                }
                if (!unknownAttrs.empty()) {
                    std::sort(unknownAttrs.begin(), unknownAttrs.end(),
                              [](const std::string &a, const std::string &b) {
                                  return lowercase(a) < lowercase(b);
                              });
                    std::string list;
                    for (size_t i = 0; i < unknownAttrs.size(); i++) {
                        if (i) list += ',';
                        list += uppercase(unknownAttrs[i]);
                    }
                    throw webstrada::exception(("Attribute validation error for tag CFLOG. It does not allow the attribute(s) " + list + ". The valid attribute(s) are APPLICATION,FILE,LOG,TEXT,TYPE.").c_str());
                }
                if (!hasText) {
                    throw webstrada::exception("Attribute validation error for tag CFLOG. It requires the attribute(s): TEXT.");
                }

                auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                auto compileLogValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *aText = nullptr, *aType = nullptr, *aApp = nullptr;
                llvm::Value *aFile = nullptr, *aLog = nullptr;
                for (const auto &a : logAttrs) {
                    llvm::Value *val = compileLogValue(a.second);
                    if (a.first == "text") aText = val;
                    else if (a.first == "type") aType = val;
                    else if (a.first == "application") aApp = val;
                    else if (a.first == "file") aFile = val;
                    else if (a.first == "log") aLog = val;
                }
                llvm::Value *logNull = llvm::ConstantPointerNull::get(builder.getPtrTy());
                auto *fWriteLog = getOrCreateHelper(module, builder, "cf_writelog", builder.getPtrTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                emitCall(builder, fWriteLog, {aText ? aText : logNull, aType ? aType : logNull,
                                              aApp ? aApp : logNull, aFile ? aFile : logNull,
                                              aLog ? aLog : logNull});

                // Scan for the matching `</cflog>` (tracking nested <cflog>).
                // An unterminated <cflog> is SELF-CLOSING in CF (verified: the
                // following text renders normally), so the scan only consumes a
                // body when a matching end tag exists.
                std::vector<TextParserTokenItem> logBody;
                size_t logBodyStart = token.position + token.len;
                size_t logBodyEnd = 0;
                int logDepth = 0;
                size_t logNextIdx = index + 1;
                bool logFoundEnd = false;
                if (tokenName.endsWith("/>")) {
                    logNextIdx = index + 1;
                    logFoundEnd = true;
                    logBodyEnd = logBodyStart;
                }
                for (size_t i = index + 1; !logFoundEnd && i < tokens.size(); i++) {
                    const auto &tok = tokens[i];
                    if (tok.token_id == TextParser_cfml_StartTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("<cflog") && !tn.endsWith("/>")) logDepth++;
                    } else if (tok.token_id == TextParser_cfml_EndTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("</cflog")) {
                            if (logDepth > 0) logDepth--;
                            else {
                                logBodyEnd = tok.position;
                                logNextIdx = i + 1;
                                logFoundEnd = true;
                                break;
                            }
                        }
                    }
                    if (!logFoundEnd) logBody.push_back(tok);
                }
                if (!logFoundEnd) {
                    // Unterminated <cflog>: self-closing, no body to compile.
                    logBody.clear();
                }
                if (logBodyEnd > logBodyStart || !logBody.empty()) {
                    // Compile the skipped body into a dead block (never runs).
                    llvm::BasicBlock *deadBB = llvm::BasicBlock::Create(context, "cflog.body.dead", mainfunc);
                    llvm::BasicBlock *contBB = llvm::BasicBlock::Create(context, "cflog.body.cont", mainfunc);
                    builder.CreateBr(contBB);
                    builder.SetInsertPoint(deadBB);
                    WhitespaceState wsBody(ws.enabled, ws.flag);
                    wsBody.markTag(false, false);
                    size_t bodyPos = logBodyStart;
                    size_t bidx = 0;
                    compile_token_list(logBody, bidx, bodyPos, context, module, builder, mainfunc,
                                       out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                                       cfm_text, logBodyEnd, loopStack);
                    if (bodyPos < logBodyEnd) {
                        wsBody.feed(module, builder, out, cfm_text + bodyPos, logBodyEnd - bodyPos, WsRight::Tag);
                    }
                    wsBody.finish(module, builder, out, WsRight::Tag);
                    builder.CreateBr(contBB);
                    builder.SetInsertPoint(contBB);
                }
                index = logNextIdx - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cftimer")) {
                // <cftimer [type] [label]>BODY</cftimer>: always evaluates its
                // body; the timing display (inline/comment/outline/debug) is
                // gated on debugging being enabled (config::debugEnabled), which
                // this engine leaves off — matching CF with debugging disabled
                // (the RDS host default), where no timing is displayed. Unknown
                // attributes are compile-time errors
                // ("... The valid attribute(s) are LABEL,TYPE."); the `type`
                // value is validated at runtime by cf_timer_begin (a catchable
                // Template error for an invalid value, even a static literal).
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                static const std::unordered_set<std::string> timerValidAttrs = {"label", "type"};
                std::vector<std::string> unknownAttrs;
                std::vector<std::pair<std::string, std::vector<TextParserTokenItem>>> timerAttrs;
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id) &&
                                           isAttrNameToken(*attrParts, vi, cfm_text));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (timerValidAttrs.find(anameLow.constData()) == timerValidAttrs.end()) {
                        unknownAttrs.push_back(aname.constData());
                    } else {
                        timerAttrs.push_back({anameLow.constData(), valToks});
                    }
                    ai = vi;
                }
                if (!unknownAttrs.empty()) {
                    std::sort(unknownAttrs.begin(), unknownAttrs.end(),
                              [](const std::string &a, const std::string &b) {
                                  return lowercase(a) < lowercase(b);
                              });
                    std::string list;
                    for (size_t i = 0; i < unknownAttrs.size(); i++) {
                        if (i) list += ',';
                        list += uppercase(unknownAttrs[i]);
                    }
                    throw webstrada::exception(("Attribute validation error for tag CFTIMER. It does not allow the attribute(s) " + list + ". The valid attribute(s) are LABEL,TYPE.").c_str());
                }

                auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                auto compileTimerValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *aType = nullptr, *aLabel = nullptr;
                for (const auto &a : timerAttrs) {
                    llvm::Value *val = compileTimerValue(a.second);
                    if (a.first == "type") aType = val;
                    else if (a.first == "label") aLabel = val;
                }
                llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                auto *fTimerBegin = getOrCreateHelper(module, builder, "cf_timer_begin", builder.getInt64Ty(), {builder.getPtrTy()});
                llvm::Value *start = emitCall(builder, fTimerBegin, {aType ? aType : nullPtr});

                // Scan for the matching `</cftimer>` (tracking nested ones);
                // the body is evaluated. An unterminated <cftimer> is a
                // compile-time error in CF ("Context validation error for tag
                // CFTIMER. The start tag must have a matching end tag. ..."),
                // unlike <cflog>/<cftrace> which are self-closing.
                std::vector<TextParserTokenItem> timerBody;
                size_t timerBodyStart = token.position + token.len;
                size_t timerBodyEnd = 0;
                int timerDepth = 0;
                size_t timerNextIdx = index + 1;
                bool timerFoundEnd = false;
                if (tokenName.endsWith("/>")) {
                    timerNextIdx = index + 1;
                    timerFoundEnd = true;
                    timerBodyEnd = timerBodyStart;
                }
                for (size_t i = index + 1; !timerFoundEnd && i < tokens.size(); i++) {
                    const auto &tok = tokens[i];
                    if (tok.token_id == TextParser_cfml_StartTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("<cftimer") && !tn.endsWith("/>")) timerDepth++;
                    } else if (tok.token_id == TextParser_cfml_EndTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("</cftimer")) {
                            if (timerDepth > 0) timerDepth--;
                            else {
                                timerBodyEnd = tok.position;
                                timerNextIdx = i + 1;
                                timerFoundEnd = true;
                                break;
                            }
                        }
                    }
                    if (!timerFoundEnd) timerBody.push_back(tok);
                }
                if (!timerFoundEnd) {
                    throw webstrada::exception("Context validation error for tag CFTIMER.",
                        "The start tag must have a matching end tag.  An explicit end tag can be provided by adding </CFTIMER>.  If the body of the tag is empty you can use the shortcut <CFTIMER .../>.");
                }
                WhitespaceState wsBody(ws.enabled, ws.flag);
                wsBody.markTag(false, false);
                size_t bodyPos = timerBodyStart;
                size_t bidx = 0;
                compile_token_list(timerBody, bidx, bodyPos, context, module, builder, mainfunc,
                                   out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                                   cfm_text, timerBodyEnd, loopStack);
                if (bodyPos < timerBodyEnd) {
                    wsBody.feed(module, builder, out, cfm_text + bodyPos, timerBodyEnd - bodyPos, WsRight::Tag);
                }
                wsBody.finish(module, builder, out, WsRight::Tag);

                auto *fTimerEnd = getOrCreateHelper(module, builder, "cf_timer_end", builder.getVoidTy(),
                    {builder.getInt64Ty(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                emitCall(builder, fTimerEnd, {start, aType ? aType : nullPtr, aLabel ? aLabel : nullPtr, out});
                index = timerNextIdx - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cftrace")) {
                // <cftrace [text] [type] [category] [inline] [abort] [var]> is a
                // complete no-op when debugging is disabled (this engine's
                // state and the RDS host default): no inline output, no log
                // write, and abort="true" does not fire (all verified on CF
                // 2025). Its body IS evaluated (verified: body text renders).
                // The attribute expressions are still evaluated — CF's setters
                // run even with debugging off, so `text="#undef#"` throws.
                // Unknown attributes are compile-time errors ("... The valid
                // attribute(s) are ABORT,CATEGORY,INLINE,TEXT,TYPE,VAR.").
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                static const std::unordered_set<std::string> traceValidAttrs = {
                    "text", "type", "category", "inline", "abort", "var"};
                std::vector<std::string> unknownAttrs;
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (!isAttrNameToken(*attrParts, ai, cfm_text)) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id) &&
                                           isAttrNameToken(*attrParts, vi, cfm_text));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (traceValidAttrs.find(anameLow.constData()) == traceValidAttrs.end()) {
                        unknownAttrs.push_back(aname.constData());
                    } else {
                        // Evaluate the value (CF's setters run even with
                        // debugging disabled) and discard the result.
                        auto *fStr = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                        auto compileTraceValue = [&](const std::vector<TextParserTokenItem> &vtoks) -> llvm::Value* {
                            if (vtoks.empty()) return nullptr;
                            if (vtoks.size() == 1 &&
                                (vtoks[0].token_id == TextParser_cfml_Variable ||
                                 vtoks[0].token_id == TextParser_cfml_Number ||
                                 vtoks[0].token_id == TextParser_cfml_Boolean)) {
                                string raw(cfm_text + vtoks[0].position, vtoks[0].len);
                                return emitCall(builder, fStr,
                                    {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                            }
                            auto ast = parseTokensToAST(vtoks, cfm_text);
                            return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                                  application, session, url, form, variables, cfm_text);
                        };
                        compileTraceValue(valToks);
                    }
                    ai = vi;
                }
                if (!unknownAttrs.empty()) {
                    std::sort(unknownAttrs.begin(), unknownAttrs.end(),
                              [](const std::string &a, const std::string &b) {
                                  return lowercase(a) < lowercase(b);
                              });
                    std::string list;
                    for (size_t i = 0; i < unknownAttrs.size(); i++) {
                        if (i) list += ',';
                        list += uppercase(unknownAttrs[i]);
                    }
                    throw webstrada::exception(("Attribute validation error for tag CFTRACE. It does not allow the attribute(s) " + list + ". The valid attribute(s) are ABORT,CATEGORY,INLINE,TEXT,TYPE,VAR.").c_str());
                }

                // Scan for the matching `</cftrace>`; the body is evaluated.
                // An unterminated <cftrace> is self-closing in CF (verified).
                std::vector<TextParserTokenItem> traceBody;
                size_t traceBodyStart = token.position + token.len;
                size_t traceBodyEnd = 0;
                int traceDepth = 0;
                size_t traceNextIdx = index + 1;
                bool traceFoundEnd = false;
                if (tokenName.endsWith("/>")) {
                    traceNextIdx = index + 1;
                    traceFoundEnd = true;
                    traceBodyEnd = traceBodyStart;
                }
                for (size_t i = index + 1; !traceFoundEnd && i < tokens.size(); i++) {
                    const auto &tok = tokens[i];
                    if (tok.token_id == TextParser_cfml_StartTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("<cftrace") && !tn.endsWith("/>")) traceDepth++;
                    } else if (tok.token_id == TextParser_cfml_EndTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("</cftrace")) {
                            if (traceDepth > 0) traceDepth--;
                            else {
                                traceBodyEnd = tok.position;
                                traceNextIdx = i + 1;
                                traceFoundEnd = true;
                                break;
                            }
                        }
                    }
                    if (!traceFoundEnd) traceBody.push_back(tok);
                }
                if (!traceFoundEnd) {
                    // Unterminated <cftrace>: self-closing, no body to compile.
                    traceBody.clear();
                }
                WhitespaceState wsBody(ws.enabled, ws.flag);
                wsBody.markTag(false, false);
                size_t bodyPos = traceBodyStart;
                size_t bidx = 0;
                compile_token_list(traceBody, bidx, bodyPos, context, module, builder, mainfunc,
                                   out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                                   cfm_text, traceBodyEnd, loopStack);
                if (bodyPos < traceBodyEnd) {
                    wsBody.feed(module, builder, out, cfm_text + bodyPos, traceBodyEnd - bodyPos, WsRight::Tag);
                }
                wsBody.finish(module, builder, out, WsRight::Tag);
                index = traceNextIdx - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfloginuser")) {
                // <cfloginuser name=.. password=.. roles=..> identifies the user
                // and binds it to the enclosing <cflogin> (or the request
                // directly when used outside one). All three attributes are
                // required at compile time; unknown attributes are compile-time
                // errors (CF 2025 messages, verified on the RDS host).
                const std::vector<TextParserTokenItem> *luAttrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        luAttrParts = &ch.children;
                        break;
                    }
                }
                static const std::unordered_set<std::string> luValidAttrs = {"name", "password", "roles"};
                std::vector<std::string> luUnknown;
                bool luHasName = false, luHasPassword = false, luHasRoles = false;
                std::vector<std::pair<std::string, std::vector<TextParserTokenItem>>> luAttrs;
                for (size_t ai = 0; ai < luAttrParts->size(); ) {
                    const auto &at = (*luAttrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < luAttrParts->size()) {
                        const auto &vt = (*luAttrParts)[vi];
                        bool nextIsAttr = (vi + 1 < luAttrParts->size() &&
                                           isOperatorToken((*luAttrParts)[vi + 1].token_id) &&
                                           isAttrNameToken(*luAttrParts, vi, cfm_text));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (luValidAttrs.find(anameLow.constData()) == luValidAttrs.end()) {
                        luUnknown.push_back(aname.constData());
                    } else {
                        if (anameLow.equals("name")) luHasName = true;
                        else if (anameLow.equals("password")) luHasPassword = true;
                        else if (anameLow.equals("roles")) luHasRoles = true;
                        luAttrs.push_back({anameLow.constData(), valToks});
                    }
                    ai = vi;
                }
                if (!luUnknown.empty()) {
                    // CF 2025: unknown attributes -> "Attribute validation
                    // error for the loginUser tag."
                    throw webstrada::exception("Attribute validation error for the loginUser tag.");
                }
                if (!luHasName || !luHasPassword || !luHasRoles) {
                    // CF 2025: missing required -> "Attribute validation error
                    // for cfloginUser."
                    throw webstrada::exception("Attribute validation error for cfloginUser.");
                }
                auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(),
                                                        {builder.getPtrTy()});
                auto compileLuValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *luName = nullptr, *luPassword = nullptr, *luRoles = nullptr;
                for (const auto &a : luAttrs) {
                    llvm::Value *val = compileLuValue(a.second);
                    if (a.first == "name") luName = val;
                    else if (a.first == "password") luPassword = val;
                    else if (a.first == "roles") luRoles = val;
                }
                auto *fLoginUser = getOrCreateHelper(module, builder, "cf_loginuser", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                auto *luNull = llvm::ConstantPointerNull::get(builder.getPtrTy());
                emitCall(builder, fLoginUser, {out, cgi, server, cookie, application, session,
                                               url, form, variables,
                                               luName ? luName : luNull,
                                               luPassword ? luPassword : luNull,
                                               luRoles ? luRoles : luNull});

            } else if (tagNameLow.startWith("<cflogout")) {
                // <cflogout [session] [applicationtoken]> logs the user out.
                // Unknown attributes are compile-time errors; a statically
                // invalid `session` value throws CF's catchable Application
                // "Attribute validation error for the logout tag." (the taglib
                // attribute validation, like CF's LogoutTag.setSession).
                const std::vector<TextParserTokenItem> *loAttrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        loAttrParts = &ch.children;
                        break;
                    }
                }
                static const std::unordered_set<std::string> loValidAttrs = {"session", "applicationtoken"};
                std::vector<std::string> loUnknown;
                std::vector<std::pair<std::string, std::vector<TextParserTokenItem>>> loAttrs;
                for (size_t ai = 0; ai < loAttrParts->size(); ) {
                    const auto &at = (*loAttrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < loAttrParts->size()) {
                        const auto &vt = (*loAttrParts)[vi];
                        bool nextIsAttr = (vi + 1 < loAttrParts->size() &&
                                           isOperatorToken((*loAttrParts)[vi + 1].token_id) &&
                                           isAttrNameToken(*loAttrParts, vi, cfm_text));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (loValidAttrs.find(anameLow.constData()) == loValidAttrs.end()) {
                        loUnknown.push_back(aname.constData());
                    } else {
                        loAttrs.push_back({anameLow.constData(), valToks});
                    }
                    ai = vi;
                }
                if (!loUnknown.empty()) {
                    throw webstrada::exception("Attribute validation error for the logout tag.");
                }
                auto *fLoCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(),
                                                          {builder.getPtrTy()});
                auto compileLoValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fLoCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *loSession = nullptr, *loAppToken = nullptr;
                for (const auto &a : loAttrs) {
                    llvm::Value *val = compileLoValue(a.second);
                    if (a.first == "session") loSession = val;
                    else if (a.first == "applicationtoken") loAppToken = val;
                }
                auto *fLogout = getOrCreateHelper(module, builder, "cf_logout", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                auto *loNull = llvm::ConstantPointerNull::get(builder.getPtrTy());
                emitCall(builder, fLogout, {out, cgi, server, cookie, application, session,
                                            url, form, variables,
                                            loSession ? loSession : loNull,
                                            loAppToken ? loAppToken : loNull});

            } else if (tagNameLow.startWith("<cflogin")) {
                // <cflogin [idletimeout] [applicationtoken] [cookiedomain]> is a
                // container: the body runs when the request has no live login,
                // and is skipped when it does (CF's AuthenticateTag SKIP_BODY).
                // A <cfloginuser> inside the body binds to this tag; at the end
                // tag the login is committed (token + cookie) or the user is
                // logged out (no <cfloginuser> ran). The `cflogin` struct
                // (j_username/j_password) is set/removed by the runtime.
                const std::vector<TextParserTokenItem> *lgAttrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        lgAttrParts = &ch.children;
                        break;
                    }
                }
                static const std::unordered_set<std::string> lgValidAttrs = {
                    "idletimeout", "usebasicauth", "allowconcurrent", "applicationtoken", "cookiedomain"};
                std::vector<std::string> lgUnknown;
                std::vector<std::pair<std::string, std::vector<TextParserTokenItem>>> lgAttrs;
                for (size_t ai = 0; ai < lgAttrParts->size(); ) {
                    const auto &at = (*lgAttrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < lgAttrParts->size()) {
                        const auto &vt = (*lgAttrParts)[vi];
                        bool nextIsAttr = (vi + 1 < lgAttrParts->size() &&
                                           isOperatorToken((*lgAttrParts)[vi + 1].token_id) &&
                                           isAttrNameToken(*lgAttrParts, vi, cfm_text));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (lgValidAttrs.find(anameLow.constData()) == lgValidAttrs.end()) {
                        lgUnknown.push_back(aname.constData());
                    } else {
                        lgAttrs.push_back({anameLow.constData(), valToks});
                    }
                    ai = vi;
                }
                if (!lgUnknown.empty()) {
                    // CF 2025: "Attribute validation error for the login tag."
                    throw webstrada::exception("Attribute validation error for the login tag.");
                }
                auto *fLgCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(),
                                                          {builder.getPtrTy()});
                auto compileLgValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fLgCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *lgIdle = nullptr, *lgBasic = nullptr, *lgConcurrent = nullptr;
                llvm::Value *lgAppToken = nullptr, *lgCookieDomain = nullptr;
                for (const auto &a : lgAttrs) {
                    llvm::Value *val = compileLgValue(a.second);
                    if (a.first == "idletimeout") lgIdle = val;
                    else if (a.first == "usebasicauth") lgBasic = val;
                    else if (a.first == "allowconcurrent") lgConcurrent = val;
                    else if (a.first == "applicationtoken") lgAppToken = val;
                    else if (a.first == "cookiedomain") lgCookieDomain = val;
                }
                auto *fLoginBegin = getOrCreateHelper(module, builder, "cf_login_begin", builder.getInt32Ty(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy()});
                auto *lgNull = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *runBody = emitCall(builder, fLoginBegin,
                    {out, cgi, server, cookie, application, session, url, form, variables,
                     lgIdle ? lgIdle : lgNull, lgBasic ? lgBasic : lgNull,
                     lgConcurrent ? lgConcurrent : lgNull,
                     lgAppToken ? lgAppToken : lgNull,
                     lgCookieDomain ? lgCookieDomain : lgNull});

                // Scan for the matching `</cflogin>` (tracking nested ones). An
                // unterminated <cflogin> takes the rest of the template, like CF.
                std::vector<TextParserTokenItem> lgBody;
                size_t lgBodyStart = token.position + token.len;
                size_t lgBodyEnd = 0;
                int lgDepth = 0;
                size_t lgNextIdx = index + 1;
                bool lgFoundEnd = false;
                if (tokenName.endsWith("/>")) {
                    lgNextIdx = index + 1;
                    lgFoundEnd = true;
                    lgBodyEnd = lgBodyStart;
                }
                for (size_t i = index + 1; !lgFoundEnd && i < tokens.size(); i++) {
                    const auto &tok = tokens[i];
                    if (tok.token_id == TextParser_cfml_StartTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("<cflogin") && !tn.startWith("<cfloginuser") &&
                            !tn.startWith("<cflogout") && !tn.endsWith("/>")) {
                            lgDepth++;
                        }
                    } else if (tok.token_id == TextParser_cfml_EndTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("</cflogin")) {
                            if (lgDepth > 0) lgDepth--;
                            else {
                                lgBodyEnd = tok.position;
                                lgNextIdx = i + 1;
                                lgFoundEnd = true;
                                break;
                            }
                        }
                    }
                    if (!lgFoundEnd) lgBody.push_back(tok);
                }
                if (!lgFoundEnd) {
                    lgBodyEnd = cfm_text_size;
                    lgNextIdx = tokens.size();
                }

                // If the body should run, compile it under an `if`. The end-tag
                // runtime always runs afterwards (commit or logout).
                llvm::BasicBlock *lgRunBB = llvm::BasicBlock::Create(context, "cflogin.body", mainfunc);
                llvm::BasicBlock *lgContBB = llvm::BasicBlock::Create(context, "cflogin.cont", mainfunc);
                llvm::Value *runBodyBool = builder.CreateICmpNE(runBody, builder.getInt32(0));
                builder.CreateCondBr(runBodyBool, lgRunBB, lgContBB);
                builder.SetInsertPoint(lgRunBB);
                if (lgBodyEnd > lgBodyStart || !lgBody.empty()) {
                    WhitespaceState wsBody(ws.enabled, ws.flag);
                    wsBody.markTag(false, false);
                    size_t bodyPos = lgBodyStart;
                    size_t bidx = 0;
                    compile_token_list(lgBody, bidx, bodyPos, context, module, builder, mainfunc,
                                       out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                                       cfm_text, lgBodyEnd, loopStack);
                    if (bodyPos < lgBodyEnd) {
                        wsBody.feed(module, builder, out, cfm_text + bodyPos, lgBodyEnd - bodyPos, WsRight::Tag);
                    }
                    wsBody.finish(module, builder, out, WsRight::Tag);
                }
                builder.CreateBr(lgContBB);
                builder.SetInsertPoint(lgContBB);

                auto *fLoginEnd = getOrCreateHelper(module, builder, "cf_login_end", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty()});
                emitCall(builder, fLoginEnd,
                    {out, cgi, server, cookie, application, session, url, form, variables,
                     lgIdle ? lgIdle : lgNull, lgBasic ? lgBasic : lgNull,
                     lgConcurrent ? lgConcurrent : lgNull,
                     lgAppToken ? lgAppToken : lgNull,
                     lgCookieDomain ? lgCookieDomain : lgNull,
                     runBody});
                index = lgNextIdx - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfparam")) {
                // <cfparam name=".." [type] [default] [min] [max] [maxlength]
                // [pattern]> tests a parameter and optionally assigns a default.
                // Compile-time attribute validation reproduces CF 2025: unknown
                // attributes and a missing `name` abort compilation. Attribute
                // values are compiled like <cfapplication> so `default="#x#"`
                // evaluates (the default expression is ALWAYS evaluated, even
                // when the parameter exists — CF's ParamTag setters run before
                // doStartTag). The runtime helper cf_param (src/cftags/
                // tag_param.cpp) finds-or-assigns and validates.
                const std::vector<TextParserTokenItem> *paramAttrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        paramAttrParts = &ch.children;
                        break;
                    }
                }
                static const std::unordered_set<std::string> paramValidAttrs = {
                    "name", "type", "default", "min", "max", "maxlength", "pattern"};
                std::vector<std::string> paramUnknownAttrs;
                std::vector<std::pair<std::string, std::vector<TextParserTokenItem>>> paramAttrs;
                bool hasName = false;
                for (size_t ai = 0; ai < paramAttrParts->size(); ) {
                    const auto &at = (*paramAttrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < paramAttrParts->size()) {
                        const auto &vt = (*paramAttrParts)[vi];
                        bool nextIsAttr = (vi + 1 < paramAttrParts->size() &&
                                           isOperatorToken((*paramAttrParts)[vi + 1].token_id) &&
                                           isAttrNameToken(*paramAttrParts, vi, cfm_text));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (paramValidAttrs.find(anameLow.constData()) == paramValidAttrs.end()) {
                        paramUnknownAttrs.push_back(aname.constData());
                    } else {
                        if (anameLow.equals("name")) hasName = true;
                        paramAttrs.push_back({anameLow.constData(), valToks});
                    }
                    ai = vi;
                }
                if (!paramUnknownAttrs.empty()) {
                    std::sort(paramUnknownAttrs.begin(), paramUnknownAttrs.end(),
                              [](const std::string &a, const std::string &b) {
                                  return lowercase(a) < lowercase(b);
                              });
                    std::string list;
                    for (size_t i = 0; i < paramUnknownAttrs.size(); i++) {
                        if (i) list += ',';
                        list += uppercase(paramUnknownAttrs[i]);
                    }
                    throw webstrada::exception(("Attribute validation error for tag CFPARAM. It does not allow the attribute(s) " + list + ". The valid attribute(s) are DEFAULT,MAX,MAXLENGTH,MIN,NAME,PATTERN,TYPE.").c_str());
                }
                if (!hasName) {
                    throw webstrada::exception("Attribute validation error for tag CFPARAM. It requires the attribute(s): NAME.");
                }

                auto *paramCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                auto compileParamValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, paramCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *aParamName = nullptr, *aParamType = nullptr, *aParamDefault = nullptr;
                llvm::Value *aParamMin = nullptr, *aParamMax = nullptr, *aParamMaxlen = nullptr;
                llvm::Value *aParamPattern = nullptr;
                for (const auto &a : paramAttrs) {
                    llvm::Value *val = compileParamValue(a.second);
                    if (a.first == "name") aParamName = val;
                    else if (a.first == "type") aParamType = val;
                    else if (a.first == "default") aParamDefault = val;
                    else if (a.first == "min") aParamMin = val;
                    else if (a.first == "max") aParamMax = val;
                    else if (a.first == "maxlength") aParamMaxlen = val;
                    else if (a.first == "pattern") aParamPattern = val;
                }
                llvm::Value *paramNull = llvm::ConstantPointerNull::get(builder.getPtrTy());
                auto *fParam = getOrCreateHelper(module, builder, "cf_param", builder.getPtrTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy()});
                llvm::Value *paramArgs[] = {
                    aParamName ? aParamName : paramNull,
                    aParamType ? aParamType : paramNull,
                    aParamDefault ? aParamDefault : paramNull,
                    aParamMin ? aParamMin : paramNull,
                    aParamMax ? aParamMax : paramNull,
                    aParamMaxlen ? aParamMaxlen : paramNull,
                    aParamPattern ? aParamPattern : paramNull,
                    cgi, server, cookie, application, session, url, form, variables
                };
                emitCall(builder, fParam, paramArgs);

                // CF's ParamTag doStartTag returns SKIP_BODY: a
                // <cfparam>...</cfparam> body is consumed but never executed
                // (verified on CF 2025: a <cfset> in the body has no effect).
                // Like <cflog>, the body is still compiled into a dead block so
                // a body syntax error surfaces at compile time.
                std::vector<TextParserTokenItem> paramBody;
                size_t paramBodyStart = token.position + token.len;
                size_t paramBodyEnd = 0;
                int paramDepth = 0;
                size_t paramNextIdx = index + 1;
                bool paramFoundEnd = false;
                if (tokenName.endsWith("/>")) {
                    paramNextIdx = index + 1;
                    paramFoundEnd = true;
                    paramBodyEnd = paramBodyStart;
                }
                for (size_t i = index + 1; !paramFoundEnd && i < tokens.size(); i++) {
                    const auto &tok = tokens[i];
                    if (tok.token_id == TextParser_cfml_StartTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("<cfparam") && !tn.endsWith("/>")) paramDepth++;
                    } else if (tok.token_id == TextParser_cfml_EndTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("</cfparam")) {
                            if (paramDepth > 0) paramDepth--;
                            else {
                                paramBodyEnd = tok.position;
                                paramNextIdx = i + 1;
                                paramFoundEnd = true;
                                break;
                            }
                        }
                    }
                    if (!paramFoundEnd) paramBody.push_back(tok);
                }
                if (!paramFoundEnd) {
                    paramBody.clear();
                }
                if (paramBodyEnd > paramBodyStart || !paramBody.empty()) {
                    llvm::BasicBlock *paramDeadBB = llvm::BasicBlock::Create(context, "cfparam.body.dead", mainfunc);
                    llvm::BasicBlock *paramContBB = llvm::BasicBlock::Create(context, "cfparam.body.cont", mainfunc);
                    builder.CreateBr(paramContBB);
                    builder.SetInsertPoint(paramDeadBB);
                    WhitespaceState wsBody(ws.enabled, ws.flag);
                    wsBody.markTag(false, false);
                    size_t bodyPos = paramBodyStart;
                    size_t bidx = 0;
                    compile_token_list(paramBody, bidx, bodyPos, context, module, builder, mainfunc,
                                       out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                                       cfm_text, paramBodyEnd, loopStack);
                    if (bodyPos < paramBodyEnd) {
                        wsBody.feed(module, builder, out, cfm_text + bodyPos, paramBodyEnd - bodyPos, WsRight::Tag);
                    }
                    wsBody.finish(module, builder, out, WsRight::Tag);
                    builder.CreateBr(paramContBB);
                    builder.SetInsertPoint(paramContBB);
                }
                index = paramNextIdx - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfobjectcache")) {
                // <cfobjectcache action="clear"> flushes the query cache
                // (CF's ObjectCacheTag -> purgeQueryCache). `action` defaults
                // to clear. Compile-time validation: unknown attributes are
                // errors; a static bad action value is a compile-time error.
                const std::vector<TextParserTokenItem> *ocAttrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        ocAttrParts = &ch.children;
                        break;
                    }
                }
                static const std::unordered_set<std::string> ocValidAttrs = {"action"};
                std::vector<std::string> ocUnknownAttrs;
                llvm::Value *ocAction = nullptr;
                bool ocActionIsStatic = false;
                std::string ocStaticAction;
                auto ocStaticLiteral = [&](const std::vector<TextParserTokenItem> &toks, std::string &out) -> bool {
                    if (toks.size() != 1) return false;
                    const auto &t = toks[0];
                    if (t.token_id != TextParser_cfml_DoubleString &&
                        t.token_id != TextParser_cfml_SingleString) {
                        return false;
                    }
                    string raw(cfm_text + t.position, t.len);
                    std::string content;
                    if (raw.length() >= 2) content.assign(raw.constData() + 1, raw.length() - 2);
                    if (content.find('#') != std::string::npos) return false;
                    out = content;
                    return true;
                };
                for (size_t ai = 0; ai < ocAttrParts->size(); ) {
                    const auto &at = (*ocAttrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < ocAttrParts->size()) {
                        const auto &vt = (*ocAttrParts)[vi];
                        bool nextIsAttr = (vi + 1 < ocAttrParts->size() &&
                                           isOperatorToken((*ocAttrParts)[vi + 1].token_id) &&
                                           isAttrNameToken(*ocAttrParts, vi, cfm_text));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (ocValidAttrs.find(anameLow.constData()) == ocValidAttrs.end()) {
                        ocUnknownAttrs.push_back(aname.constData());
                    } else {
                        if (ocStaticLiteral(valToks, ocStaticAction)) {
                            ocActionIsStatic = true;
                        }
                        if (!valToks.empty()) {
                            auto *fStr = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                            if (ocActionIsStatic) {
                                ocAction = emitCall(builder, fStr,
                                    {builder.CreateGlobalString(llvm::StringRef(ocStaticAction.c_str(), ocStaticAction.length()), "", 0, module, true)});
                            } else {
                                auto ast = parseTokensToAST(valToks, cfm_text);
                                ocAction = CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                                          application, session, url, form, variables, cfm_text);
                            }
                        }
                    }
                    ai = vi;
                }
                if (!ocUnknownAttrs.empty()) {
                    std::sort(ocUnknownAttrs.begin(), ocUnknownAttrs.end(),
                              [](const std::string &a, const std::string &b) {
                                  return lowercase(a) < lowercase(b);
                              });
                    std::string list;
                    for (size_t i = 0; i < ocUnknownAttrs.size(); i++) {
                        if (i) list += ',';
                        list += lowercase(ocUnknownAttrs[i]);
                    }
                    throw webstrada::exception(("Attribute validation error for the objectcache tag. The tag does not have an attribute called " + list + ". The valid attribute(s) are action.").c_str());
                }
                if (!ocActionIsStatic && ocAction == nullptr) {
                    // action is required (CF: "The tag requires the ACTION attribute.").
                    throw webstrada::exception("Attribute validation error for the CFOBJECTCACHE tag.",
                        "The tag requires the ACTION attribute.");
                }
                if (ocActionIsStatic) {
                    std::string al = ocStaticAction;
                    for (char &c : al) c = tolower(c);
                    std::string shown = ocStaticAction.empty() ? "''" : ocStaticAction;
                    if (al != "clear") {
                        throw webstrada::exception("Attribute validation error for CFOBJECTCACHE.",
                            ("The value of the ACTION attribute, which is currently " + shown + ", must be one of the values: CLEAR.").c_str());
                    }
                }
                llvm::Value *ocNull = llvm::ConstantPointerNull::get(builder.getPtrTy());
                auto *fObjCache = getOrCreateHelper(module, builder, "cf_objectcache", builder.getVoidTy(),
                    {builder.getPtrTy()});
                emitCall(builder, fObjCache, {ocAction ? ocAction : ocNull});

                // Like <cfparam>, a <cfobjectcache>...</cfobjectcache> body is
                // consumed but never executed (CF's GenericTag doStartTag
                // returns SKIP_BODY — verified on CF 2025: a <cfset> in the
                // body has no effect). The body is still compiled into a dead
                // block so a body syntax error surfaces at compile time.
                std::vector<TextParserTokenItem> ocBody;
                size_t ocBodyStart = token.position + token.len;
                size_t ocBodyEnd = 0;
                int ocDepth = 0;
                size_t ocNextIdx = index + 1;
                bool ocFoundEnd = false;
                if (tokenName.endsWith("/>")) {
                    ocNextIdx = index + 1;
                    ocFoundEnd = true;
                    ocBodyEnd = ocBodyStart;
                }
                for (size_t i = index + 1; !ocFoundEnd && i < tokens.size(); i++) {
                    const auto &tok = tokens[i];
                    if (tok.token_id == TextParser_cfml_StartTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("<cfobjectcache") && !tn.endsWith("/>")) ocDepth++;
                    } else if (tok.token_id == TextParser_cfml_EndTag) {
                        string tn(cfm_text + tok.position, tok.len);
                        tn.toLower();
                        if (tn.startWith("</cfobjectcache")) {
                            if (ocDepth > 0) ocDepth--;
                            else {
                                ocBodyEnd = tok.position;
                                ocNextIdx = i + 1;
                                ocFoundEnd = true;
                                break;
                            }
                        }
                    }
                    if (!ocFoundEnd) ocBody.push_back(tok);
                }
                if (!ocFoundEnd) {
                    ocBody.clear();
                }
                if (ocBodyEnd > ocBodyStart || !ocBody.empty()) {
                    llvm::BasicBlock *ocDeadBB = llvm::BasicBlock::Create(context, "cfobjectcache.body.dead", mainfunc);
                    llvm::BasicBlock *ocContBB = llvm::BasicBlock::Create(context, "cfobjectcache.body.cont", mainfunc);
                    builder.CreateBr(ocContBB);
                    builder.SetInsertPoint(ocDeadBB);
                    WhitespaceState wsBody(ws.enabled, ws.flag);
                    wsBody.markTag(false, false);
                    size_t bodyPos = ocBodyStart;
                    size_t bidx = 0;
                    compile_token_list(ocBody, bidx, bodyPos, context, module, builder, mainfunc,
                                       out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                                       cfm_text, ocBodyEnd, loopStack);
                    if (bodyPos < ocBodyEnd) {
                        wsBody.feed(module, builder, out, cfm_text + bodyPos, ocBodyEnd - bodyPos, WsRight::Tag);
                    }
                    wsBody.finish(module, builder, out, WsRight::Tag);
                    builder.CreateBr(ocContBB);
                    builder.SetInsertPoint(ocContBB);
                }
                index = ocNextIdx - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfapplication")) {
                // `<cfapplication>` enables the APPLICATION (and optional
                // SESSION) scopes. Attribute values are compiled from the tag's
                // Expression children exactly like <cfthrow>, so
                // `applicationtimeout="#CreateTimeSpan(1,0,0,0)#"` evaluates.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto *fEnable = getOrCreateHelper(module, builder, "cf_application_enable", builder.getPtrTy(),
                                                  std::vector<llvm::Type*>(8, builder.getPtrTy()));
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder,
                            getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()}),
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };

                llvm::Value *nameVal = nullptr, *sessionMgmtVal = nullptr;
                llvm::Value *appTimeoutVal = nullptr, *sessionTimeoutVal = nullptr, *setClientCookiesVal = nullptr;
                llvm::Value *searchImplicitScopesVal = nullptr;
                llvm::Value *loginStorageVal = nullptr;
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    aname.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (!valToks.empty()) {
                        llvm::Value *val = compileValue(valToks);
                        if (aname.equals("name")) nameVal = val;
                        else if (aname.equals("sessionmanagement")) sessionMgmtVal = val;
                        else if (aname.equals("applicationtimeout")) appTimeoutVal = val;
                        else if (aname.equals("sessiontimeout")) sessionTimeoutVal = val;
                        else if (aname.equals("setclientcookies")) setClientCookiesVal = val;
                        else if (aname.equals("searchimplicitscopes")) searchImplicitScopesVal = val;
                        else if (aname.equals("loginstorage")) loginStorageVal = val;
                    }
                    ai = vi;
                }

                // <cfapplication searchimplicitscopes="yes|true"> enables the
                // implicit-scope search for unqualified names (CGI, URL, FORM,
                // COOKIE, ...). Default false, matching ColdFusion.
                if (searchImplicitScopesVal) {
                    auto *fSetSearch = getOrCreateHelper(module, builder, "cf_set_search_implicit_scopes", builder.getVoidTy(),
                                                         {builder.getPtrTy()});
                    emitCall(builder, fSetSearch, {searchImplicitScopesVal});
                }

                // <cfapplication loginstorage="session"> stores the login key in
                // the session scope instead of the CFAUTHORIZATION cookie.
                if (loginStorageVal) {
                    auto *fSetLoginStorage = getOrCreateHelper(module, builder, "cf_set_login_storage", builder.getVoidTy(),
                                                               {builder.getPtrTy()});
                    emitCall(builder, fSetLoginStorage, {loginStorageVal});
                }

                auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *args[] = {
                    application, session, cookie,
                    nameVal ? nameVal : nullPtr,
                    sessionMgmtVal ? sessionMgmtVal : nullPtr,
                    appTimeoutVal ? appTimeoutVal : nullPtr,
                    sessionTimeoutVal ? sessionTimeoutVal : nullPtr,
                    setClientCookiesVal ? setClientCookiesVal : nullPtr
                };
                emitCall(builder, fEnable, args);
            } else if (tagNameLow.startWith("<cfimage")) {
                // `<cfimage>` performs image actions (read/write/convert/
                // resize/border/rotate/info/writetobrowser/captcha). Attribute
                // values are compiled from the tag's Expression children like
                // <cfapplication>/<cfthrow>, so `source="#img#"` evaluates and
                // `width="200%"` stays a literal string. The runtime helper
                // `cf_cfimage` (src/cftags/tag_image.cpp) does the work.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
                auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                                       {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                auto *fCfimage = getOrCreateHelper(module, builder, "cf_cfimage", builder.getPtrTy(),
                                                   {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fCreateString,
                                        {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *attrsVal = emitCall(builder, fCreateStruct, {});
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    aname.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (!valToks.empty()) {
                        llvm::Value *val = compileValue(valToks);
                        if (val) {
                            auto *keyStr = builder.CreateGlobalString(llvm::StringRef(aname.constData(), aname.length()), "", 0, module, true);
                            emitCall(builder, fIndexAssign, {attrsVal,
                                emitCall(builder, fCreateString, {keyStr}), val});
                        }
                    }
                    ai = vi;
                }
                emitCall(builder, fCfimage, {attrsVal, variables, out});
            } else if (tagNameLow.startWith("<cfthrow")) {
                // `<cfthrow>` raises an exception from its attributes. Attribute
                // values are compiled from the tag's Expression children, so a
                // quoted value interpolates #expr# (`message="at#i#"` -> "at2")
                // and a bare unquoted word stays a literal string (CF tag
                // semantics). Unknown attributes are ignored, matching CF.
                auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
                auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                                       {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                auto *fThrow = getOrCreateHelper(module, builder, "cf_eh_throw_new", builder.getVoidTy(),
                                                 {builder.getPtrTy(), builder.getInt32Ty()});

                llvm::Value *exStruct = emitCall(builder, fCreateStruct, {});

                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto setKey = [&](const char *key, llvm::Value *val) {
                    emitCall(builder, fIndexAssign, {exStruct,
                        emitCall(builder, fCreateString, {builder.CreateGlobalString(key, "", 0, module, true)}), val});
                };
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fCreateString, {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };

                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    aname.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (!valToks.empty()) {
                        llvm::Value *val = compileValue(valToks);
                        if (aname.equals("type")) setKey("TYPE", val);
                        else if (aname.equals("message")) setKey("MESSAGE", val);
                        else if (aname.equals("detail")) setKey("DETAIL", val);
                        else if (aname.equals("errorcode")) setKey("ERRORCODE", val);
                        else if (aname.equals("extendedinfo")) setKey("EXTENDEDINFO", val);
                    }
                    ai = vi;
                }

                emitCall(builder, fThrow, {exStruct, builder.getInt32(0)});
                builder.CreateUnreachable();
                auto deadBB = llvm::BasicBlock::Create(context, "cfthrow.cont", mainfunc);
                builder.SetInsertPoint(deadBB);
            } else if (tagNameLow.startWith("<cfinclude")) {
                // `<cfinclude>` runs another CFML template in the current
                // request, sharing all scopes. Attribute values are compiled
                // from the tag's Expression children like <cfthrow>, so
                // `template="#path#"` evaluates and `runonce="true"` becomes a
                // boolean value; the runtime helper cf_include does the path
                // resolution, caching and execution.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        auto *fStr = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                        return emitCall(builder, fStr,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *templateVal = nullptr, *runonceVal = nullptr;
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    if (!anameLow.equals("template") && !anameLow.equals("runonce")) {
                        string detail = "The tag does not have an attribute called ";
                        detail += aname;
                        detail += ". The valid attribute(s) are template, runonce.";
                        throw webstrada::exception("Attribute validation error for the cfinclude tag.", detail);
                    }
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (!valToks.empty()) {
                        llvm::Value *val = compileValue(valToks);
                        if (anameLow.equals("template")) templateVal = val;
                        else if (anameLow.equals("runonce")) runonceVal = val;
                    }
                    ai = vi;
                }
                if (!templateVal) {
                    throw webstrada::exception("cfinclude", "Missing required attribute (template).");
                }
                auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                auto *fInclude = getOrCreateHelper(module, builder, "cf_include", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                emitCall(builder, fInclude, {out, cgi, server, cookie, application, session,
                                             url, form, variables, templateVal,
                                             runonceVal ? runonceVal : nullPtr});
            } else if (tagNameLow.startWith("<cferror")) {
                // `<cferror>` registers an error handler for the current
                // request. Attribute validation is at compile time like
                // <cfinclude>/<cfexit>: unknown attributes and missing
                // required attributes (type, template) fail the page at
                // translation (uncatchable in CF). Attribute VALUES are
                // runtime expressions; the runtime helper does the template
                // resolution (the template must exist), the type dispatch and
                // the exception-name class mapping.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto compileAttrValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        auto *fStr = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                        return emitCall(builder, fStr,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *aType = nullptr, *aTemplate = nullptr;
                llvm::Value *aMailto = nullptr, *aException = nullptr;
                bool seenType = false, seenTemplate = false;
                std::vector<std::string> unknownAttrs;
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    if (!anameLow.equals("type") && !anameLow.equals("template") &&
                        !anameLow.equals("mailto") && !anameLow.equals("exception")) {
                        unknownAttrs.push_back(std::string(aname.constData(), aname.length()));
                    }
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (!valToks.empty()) {
                        llvm::Value *val = compileAttrValue(valToks);
                        if (anameLow.equals("type")) { aType = val; seenType = true; }
                        else if (anameLow.equals("template")) { aTemplate = val; seenTemplate = true; }
                        else if (anameLow.equals("mailto")) aMailto = val;
                        else if (anameLow.equals("exception")) aException = val;
                    }
                    ai = vi;
                }
                if (!unknownAttrs.empty()) {
                    std::sort(unknownAttrs.begin(), unknownAttrs.end(),
                              [](const std::string &a, const std::string &b) { return lowercase(a) < lowercase(b); });
                    std::string list;
                    for (size_t i = 0; i < unknownAttrs.size(); i++) {
                        if (i) list += ',';
                        std::string up = unknownAttrs[i];
                        for (auto &c : up) c = (char)toupper((unsigned char)c);
                        list += up;
                    }
                    throw webstrada::exception("Attribute validation error for tag CFERROR.",
                        ("It does not allow the attribute(s) " + list + ". The valid attribute(s) are EXCEPTION,MAILTO,TEMPLATE,TYPE.").c_str());
                }
                std::vector<std::string> missing;
                if (!seenType) missing.push_back("TYPE");
                if (!seenTemplate) missing.push_back("TEMPLATE");
                if (!missing.empty()) {
                    std::sort(missing.begin(), missing.end());
                    std::string list;
                    for (size_t i = 0; i < missing.size(); i++) {
                        if (i) list += ',';
                        list += missing[i];
                    }
                    throw webstrada::exception("Attribute validation error for tag CFERROR.",
                        ("It requires the attribute(s): " + list + ".").c_str());
                }
                auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                auto *fRegister = getOrCreateHelper(module, builder, "cf_cferror_register", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy()});
                emitCall(builder, fRegister, {out, cgi, server, cookie, application, session,
                                              url, form, variables,
                                              aType ? aType : nullPtr,
                                              aTemplate ? aTemplate : nullPtr,
                                              aMailto ? aMailto : nullPtr,
                                              aException ? aException : nullPtr});
            } else if (tagNameLow.startWith("<cfobject") || tagNameLow.startWith("<cfinvoke") ||
                       tagNameLow.startWith("<cfinvokeargument")) {
                // <cfobject type="component" name=".." component="..">,
                // <cfinvoke component=".." method=".." returnvariable=".."
                // argumentcollection=".."> and the <cfinvokeargument name=".."
                // value=".."> child. Attribute values are compiled like
                // <cfinclude> (quoted values stay literals unless #...#-wrapped).
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto compileAttrValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        auto *fStr = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                        return emitCall(builder, fStr,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *aType = nullptr, *aName = nullptr, *aComponent = nullptr;
                llvm::Value *aMethod = nullptr, *aReturnVar = nullptr, *aArgColl = nullptr;
                llvm::Value *aValue = nullptr, *aOmit = nullptr;
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    llvm::Value *val = valToks.empty() ? nullptr : compileAttrValue(valToks);
                    if (anameLow.equals("type")) aType = val;
                    else if (anameLow.equals("name")) aName = val;
                    else if (anameLow.equals("component") || anameLow.equals("class")) aComponent = val;
                    else if (anameLow.equals("method")) aMethod = val;
                    else if (anameLow.equals("returnvariable")) aReturnVar = val;
                    else if (anameLow.equals("argumentcollection")) aArgColl = val;
                    else if (anameLow.equals("value")) aValue = val;
                    else if (anameLow.equals("omit")) aOmit = val;
                    ai = vi;
                }
                auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                if (tagNameLow.startWith("<cfinvokeargument")) {
                    // <cfinvokeargument> must sit inside a <cfinvoke>; `name` and
                    // `value` are required (CF's compile-time validation).
                    if (!aName) {
                        throw webstrada::exception("Attribute validation error for cfinvokeargument.",
                            "It requires the attribute(s): NAME .");
                    }
                    if (!aValue) {
                        throw webstrada::exception("Attribute validation error for CFINVOKEARGUMENT.",
                            "It requires the attribute(s): VALUE.");
                    }
                    auto *fArg = getOrCreateHelper(module, builder, "cf_cfinvoke_argument", builder.getVoidTy(),
                        {builder.getPtrTy(), builder.getPtrTy()});
                    emitCall(builder, fArg, {aName, aValue});
                    (void)aOmit;
                } else if (tagNameLow.startWith("<cfinvoke")) {
                    // <cfinvoke> ... </cfinvoke> with optional <cfinvokeargument>
                    // children. Push the call context, compile the body (whose
                    // output is discarded, like CF's buffered InvokeTag body)
                    // and perform the invoke at the end tag.
                    if (!aMethod) {
                        throw webstrada::exception("Attribute validation error for cfinvoke.",
                            "It requires the attribute(s): METHOD .");
                    }
                    string startText(cfm_text + token.position, token.len);
                    auto *fBegin = getOrCreateHelper(module, builder, "cf_cfinvoke_begin", builder.getVoidTy(),
                        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                    emitCall(builder, fBegin, {aComponent ? aComponent : nullPtr,
                                               aMethod ? aMethod : nullPtr,
                                               aReturnVar ? aReturnVar : nullPtr,
                                               aArgColl ? aArgColl : nullPtr});

                    if (!startText.endsWith("/>")) {
                        // Scan for a matching `</cfinvoke>` (tracking nested
                        // `<cfinvoke>` depth). When there is none, the tag is
                        // treated as self-closing (the invoke runs here and the
                        // rest of the template continues, matching CF); only a
                        // properly closed <cfinvoke> has a body.
                        std::vector<TextParserTokenItem> body;
                        size_t bodyStart = token.position + token.len;
                        size_t bodyEnd = 0;
                        int depth = 0;
                        size_t nextIdx = index + 1;
                        bool foundEnd = false;
                        for (size_t i = index + 1; i < tokens.size(); i++) {
                            const auto &tok = tokens[i];
                            if (tok.token_id == TextParser_cfml_StartTag) {
                                string tn(cfm_text + tok.position, tok.len);
                                tn.toLower();
                                if (tn.startWith("<cfinvoke") && !tn.startWith("<cfinvokeargument") && !tn.endsWith("/>")) {
                                    depth++;
                                }
                            } else if (tok.token_id == TextParser_cfml_EndTag) {
                                string tn(cfm_text + tok.position, tok.len);
                                tn.toLower();
                                if (tn.startWith("</cfinvoke")) {
                                    if (depth > 0) {
                                        depth--;
                                    } else {
                                        bodyEnd = tok.position;
                                        nextIdx = i + 1;
                                        foundEnd = true;
                                        break;
                                    }
                                }
                            }
                            body.push_back(tok);
                        }
                        if (foundEnd) {
                            // Redirect the body's output to a discard buffer.
                            auto *fSilentBegin = getOrCreateHelper(module, builder, "cf_silent_begin", builder.getPtrTy(), {builder.getPtrTy()});
                            auto *fSilentEnd = getOrCreateHelper(module, builder, "cf_silent_end", builder.getVoidTy(), {});
                            llvm::Value *discard = emitCall(builder, fSilentBegin, {out});
                            WhitespaceState wsBody(ws.enabled, ws.flag);
                            wsBody.markTag(false, false); // left neighbour is <cfinvoke>
                            size_t bodyPos = bodyStart;
                            size_t bidx = 0;
                            compile_token_list(body, bidx, bodyPos, context, module, builder, mainfunc,
                                               discard, wsBody, cgi, server, cookie, application, session, url, form, variables,
                                               cfm_text, bodyEnd, loopStack);
                            if (bodyPos < bodyEnd) {
                                wsBody.feed(module, builder, discard, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
                            }
                            wsBody.finish(module, builder, discard, WsRight::Tag);
                            emitCall(builder, fSilentEnd, {});
                            index = nextIdx - 1;
                            if (index < tokens.size()) {
                                pos = tokens[index].position + tokens[index].len;
                            }
                        }
                    }
                    auto *fEnd = getOrCreateHelper(module, builder, "cf_cfinvoke_end", builder.getPtrTy(),
                        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                         builder.getPtrTy()});
                    emitCall(builder, fEnd, {out, cgi, server, cookie, application, session,
                                             url, form, variables});
                } else {
                    if (!aType || !aName || !aComponent) {
                        throw webstrada::exception("cfobject", "The type, name and component attributes are required.");
                    }
                    auto *fObject = getOrCreateHelper(module, builder, "cf_cfobject", builder.getVoidTy(),
                        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                    emitCall(builder, fObject, {out, cgi, server, cookie, application, session,
                                                url, form, variables, aType, aName, aComponent});
                }
            } else if (tagNameLow.startWith("<cfrethrow")) {
                if (!g_currentCatchExn) {
                    throw webstrada::exception("cfrethrow", "<cfrethrow> is only valid inside a <cfcatch> block");
                }
                auto *fThrow = getOrCreateHelper(module, builder, "cf_eh_throw", builder.getVoidTy(), {builder.getPtrTy()});
                emitCall(builder, fThrow, {g_currentCatchExn});
                builder.CreateUnreachable();
                auto deadBB = llvm::BasicBlock::Create(context, "cfrethrow.cont", mainfunc);
                builder.SetInsertPoint(deadBB);
            } else if (tagNameLow.startWith("<cffunction")) {
                // Tag-form functions are parsed, compiled and registered into
                // the variables scope at template entry (hoisting); the page
                // pass just skips the whole block (which may sit inside control
                // flow — CF hoists those too).
                size_t scanIdx = index + 1;
                int depth = 0;
                for (; scanIdx < tokens.size(); scanIdx++) {
                    const auto &tok = tokens[scanIdx];
                    if (tok.token_id == TextParser_cfml_StartTag && tagNameOf(tok, cfm_text) == "cffunction") {
                        depth++;
                    } else if (tok.token_id == TextParser_cfml_EndTag && tagNameOf(tok, cfm_text) == "cffunction") {
                        if (depth > 0) depth--;
                        else break;
                    }
                }
                index = scanIdx;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfargument")) {
                if (!g_returnCtx) {
                    throw webstrada::exception("cfargument", "cfargument tags are only valid inside a cffunction block");
                }
                // Inside a function body the arguments were already collected by
                // parseTagFunctionDecl; the tag itself emits nothing.
            } else if (tagNameLow.startWith("<cfproperty")) {
                // <cfproperty> is a declaration, not a statement: emit nothing.
                // Used in a .cfc construction body (collected by the component
                // compiler); a stray one on a page is likewise skipped.
                pos = token.position + token.len;
            } else if (tagNameLow.startWith("<cfreturn")) {
                // Store the coerced expression in the enclosing function's
                // return slot and jump to the common exit block. A bare
                // `<cfreturn>` returns undefined.
                if (!g_returnCtx) {
                    throw webstrada::exception("cfreturn", "'cfreturn' is only valid inside a cffunction");
                }
                const std::vector<TextParserTokenItem> *exprChildren = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        exprChildren = &ch.children;
                        break;
                    }
                }
                if (!exprChildren->empty()) {
                    auto processed = mergeObjectMembers(*exprChildren);
                    auto ast = parseTokensToAST(processed, cfm_text);
                    llvm::Value *val = CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie, application, session, url, form, variables, cfm_text);
                    auto *fCoerce = module->getFunction("cf_udf_coerce_return");
                    if (!fCoerce) fCoerce = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cf_udf_coerce_return", module);
                    llvm::Value *coerced = emitCall(builder, fCoerce, {val,
                        builder.CreateGlobalString(llvm::StringRef(g_returnCtx->returnType), "", 0, module, true),
                        builder.CreateGlobalString(llvm::StringRef(g_returnCtx->funcName), "", 0, module, true)});
                    builder.CreateStore(coerced, g_returnCtx->retSlot);
                }
                builder.CreateBr(g_returnCtx->exitBB);
                auto deadBB = llvm::BasicBlock::Create(context, "cfreturn.cont", mainfunc);
                builder.SetInsertPoint(deadBB);
            } else if (tagNameLow.startWith("<cfcase")) {
                throw webstrada::exception("cfcase", "cfcase tags are only valid inside a cfswitch block");
            } else if (tagNameLow.startWith("<cfdefaultcase")) {
                throw webstrada::exception("cfdefaultcase", "cfdefaultcase tags are only valid inside a cfswitch block");
            } else if (tagNameLow.startWith("<cfcatch")) {
                throw webstrada::exception("cfcatch", "cfcatch tags are only valid inside a cftry block");
            } else if (tagNameLow.startWith("<cffinally")) {
                throw webstrada::exception("cffinally", "cffinally tags are only valid inside a cftry block");
            } else if (tagNameLow.startWith("<cfqueryparam")) {
                // `<cfqueryparam>` validates/coerces a query parameter and
                // appends its formatted SQL literal into the query capture
                // buffer (`out`, set by the enclosing <cfquery>). Attributes:
                // value, cfsqltype, maxlength, scale, null, list, separator.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return llvm::ConstantPointerNull::get(builder.getPtrTy());
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder,
                            getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()}),
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *valueVal = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *sqltypeVal = valueVal, *maxlenVal = valueVal, *scaleVal = valueVal;
                llvm::Value *nullVal = valueVal, *listVal = valueVal, *sepVal = valueVal;
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    aname.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (!valToks.empty()) {
                        llvm::Value *val = compileValue(valToks);
                        if (aname.equals("value")) valueVal = val;
                        else if (aname.equals("cfsqltype")) sqltypeVal = val;
                        else if (aname.equals("maxlength")) maxlenVal = val;
                        else if (aname.equals("scale")) scaleVal = val;
                        else if (aname.equals("null")) nullVal = val;
                        else if (aname.equals("list")) listVal = val;
                        else if (aname.equals("separator")) sepVal = val;
                    }
                    ai = vi;
                }
                auto *fQParam = getOrCreateHelper(module, builder, "cf_queryparam", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                emitCall(builder, fQParam, {out, valueVal, sqltypeVal, maxlenVal, scaleVal, nullVal, listVal, sepVal});
            } else if (tagNameLow.startWith("<cfinsert") || tagNameLow.startWith("<cfupdate") ||
                       tagNameLow.startWith("<cfdbinfo")) {
                // <cfinsert>/<cfupdate> build INSERT/UPDATE SQL from the FORM
                // scope; <cfdbinfo> stores datasource metadata in the `name`
                // variable. All three take attribute values compiled like
                // <cfthrow> (quoted values interpolate #expr#).
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
                auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                                       {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                llvm::Value *attrsVal = emitCall(builder, fCreateStruct, {});
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        return emitCall(builder, fCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    if (!valToks.empty()) {
                        llvm::Value *val = compileValue(valToks);
                        if (val) {
                            auto *keyStr = builder.CreateGlobalString(llvm::StringRef(aname.constData(), aname.length()), "", 0, module, true);
                            emitCall(builder, fIndexAssign, {attrsVal,
                                emitCall(builder, fCreateString, {keyStr}), val});
                        }
                    }
                    ai = vi;
                }
                const char *fnName = tagNameLow.startWith("<cfinsert") ? "cf_insert_tag"
                                   : tagNameLow.startWith("<cfupdate") ? "cf_update"
                                   : "cf_dbinfo";
                auto *fDbTag = getOrCreateHelper(module, builder, fnName, builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy()});
                emitCall(builder, fDbTag, {attrsVal, cgi, server, cookie, application, session, url, form, variables});
            } else if (tagNameLow.startWith("<cfzipparam")) {
                index = compile_tag_zipparam_statement(tokens, index, context, module, builder, mainfunc,
                                                       out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                       cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfzip")) {
                index = compile_tag_zip_statement(tokens, index, context, module, builder, mainfunc,
                                                  out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                  cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfdirectory")) {
                index = compile_tag_directory_statement(tokens, index, context, module, builder, mainfunc,
                                                        out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                        cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cffile")) {
                index = compile_tag_file_statement(tokens, index, context, module, builder, mainfunc,
                                                   out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                   cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfexecute")) {
                index = compile_tag_execute_statement(tokens, index, context, module, builder, mainfunc,
                                                      out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                      cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cffeed")) {
                index = compile_tag_feed_statement(tokens, index, context, module, builder, mainfunc,
                                                   out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                   cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfwddx")) {
                index = compile_tag_wddx_statement(tokens, index, context, module, builder, mainfunc,
                                                   out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                   cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfsavecontent")) {
                index = compile_tag_savecontent_statement(tokens, index, context, module, builder, mainfunc,
                                                          out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                          cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfprocessingdirective")) {
                index = compile_tag_processingdirective_statement(tokens, index, context, module, builder, mainfunc,
                                                                  out, ws, cgi, server, cookie, application, session, url, form, variables,
                                                                  cfm_text, cfm_text_size, loopStack) - 1;
                pos = token.position + token.len;
                if (index < tokens.size()) {
                    pos = tokens[index].position + tokens[index].len;
                }
            } else if (tagNameLow.startWith("<cfcookie")) {
                // <cfcookie> sets a browser cookie: validates the attribute set
                // like CF (name required, unknown attributes are compile errors)
                // and emits the Set-Cookie header + COOKIE scope entry at runtime.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto tagAttrs = parseTagAttrs(attrParts, cfm_text);

                static const std::unordered_set<std::string> cookieValidAttrs =
                    {"name", "value", "expires", "secure", "path", "domain",
                     "httponly", "encodevalue", "preservecase", "samesite"};
                std::vector<std::string> unknownAttrs;
                for (const auto &a : tagAttrs) {
                    if (cookieValidAttrs.find(lowercase(a.first)) == cookieValidAttrs.end()) {
                        unknownAttrs.push_back(a.first);
                    }
                }
                if (!unknownAttrs.empty()) {
                    std::sort(unknownAttrs.begin(), unknownAttrs.end(),
                              [](const std::string &a, const std::string &b) { return lowercase(a) < lowercase(b); });
                    std::string list;
                    for (size_t i = 0; i < unknownAttrs.size(); i++) {
                        if (i) list += ',';
                        list += uppercase(unknownAttrs[i]);
                    }
                    throw webstrada::exception(("Attribute validation error for tag CFCOOKIE. It does not allow the attribute(s) " + list + ". The valid attribute(s) are DOMAIN,ENCODEVALUE,EXPIRES,HTTPONLY,NAME,PATH,PRESERVECASE,SAMESITE,SECURE,VALUE.").c_str());
                }
                bool hasName = false;
                for (const auto &a : tagAttrs) {
                    if (lowercase(a.first) == "name") hasName = true;
                }
                if (!hasName) {
                    throw webstrada::exception("Attribute validation error for tag CFCOOKIE. It requires the attribute(s): NAME.");
                }
                llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *nameVal = nullPtr, *valueVal = nullPtr, *expiresVal = nullPtr,
                            *secureVal = nullPtr, *pathVal = nullPtr, *domainVal = nullPtr,
                            *httpOnlyVal = nullPtr, *encodeVal = nullPtr, *preserveVal = nullPtr,
                            *sameSiteVal = nullPtr;
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_DoubleString ||
                         valToks[0].token_id == TextParser_cfml_SingleString)) {
                        auto node = std::make_unique<ExprAST>();
                        node->type = ExprAST::LiteralString;
                        node->token = valToks[0];
                        return CompileExprAST(module, builder, mainfunc, node, cgi, server, cookie,
                                              application, session, url, form, variables, cfm_text);
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                for (const auto &a : tagAttrs) {
                    llvm::Value *val = compileValue(a.second);
                    std::string low = lowercase(a.first);
                    if (low == "name") nameVal = val;
                    else if (low == "value") valueVal = val;
                    else if (low == "expires") expiresVal = val;
                    else if (low == "secure") secureVal = val;
                    else if (low == "path") pathVal = val;
                    else if (low == "domain") domainVal = val;
                    else if (low == "httponly") httpOnlyVal = val;
                    else if (low == "encodevalue") encodeVal = val;
                    else if (low == "preservecase") preserveVal = val;
                    else if (low == "samesite") sameSiteVal = val;
                }
                auto *fCookie = getOrCreateHelper(module, builder, "cf_cookie_tag", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                emitCall(builder, fCookie, {cookie, nameVal, valueVal, expiresVal, secureVal,
                                            pathVal, domainVal, httpOnlyVal, encodeVal, preserveVal,
                                            sameSiteVal});
            } else if (tagNameLow.startWith("<cfhtmlhead")) {
                // <cfhtmlhead text=".."> writes its text to the response head.
                // text is required; unknown attributes are compile errors.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto tagAttrs = parseTagAttrs(attrParts, cfm_text);
                const std::vector<TextParserTokenItem> *textToks = nullptr;
                bool foundText = false;
                for (const auto &a : tagAttrs) {
                    if (lowercase(a.first) == "text") {
                        textToks = &a.second;
                        foundText = true;
                    } else {
                        throw webstrada::exception(("Attribute validation error for tag CFHTMLHEAD. It does not allow the attribute(s) " + uppercase(a.first) + ". The valid attribute(s) are TEXT.").c_str());
                    }
                }
                if (!foundText) {
                    throw webstrada::exception("Attribute validation error for tag CFHTMLHEAD. It requires the attribute(s): TEXT.");
                }
                llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *textVal = nullptr;
                if (!textToks->empty()) {
                    if (textToks->size() == 1 &&
                        ((*textToks)[0].token_id == TextParser_cfml_DoubleString ||
                         (*textToks)[0].token_id == TextParser_cfml_SingleString)) {
                        auto node = std::make_unique<ExprAST>();
                        node->type = ExprAST::LiteralString;
                        node->token = (*textToks)[0];
                        textVal = CompileExprAST(module, builder, mainfunc, node, cgi, server, cookie,
                                                 application, session, url, form, variables, cfm_text);
                    } else {
                        auto ast = parseTokensToAST(*textToks, cfm_text);
                        textVal = CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                                 application, session, url, form, variables, cfm_text);
                    }
                }
                auto *fHtmlHead = getOrCreateHelper(module, builder, "cf_htmlhead_append", builder.getVoidTy(),
                    {builder.getPtrTy()});
                emitCall(builder, fHtmlHead, {textVal ? textVal : nullPtr});
            } else if (tagNameLow.startWith("<cfsetting")) {
                // <cfsetting enablecfoutputonly=".." showdebugoutput=".."
                // requesttimeout=".."> flips the output-only mode; the other two
                // attributes are accepted (this engine has no debug output
                // section or request watchdog).
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto tagAttrs = parseTagAttrs(attrParts, cfm_text);
                static const std::unordered_set<std::string> settingValidAttrs =
                    {"enablecfoutputonly", "showdebugoutput", "requesttimeout"};
                std::vector<std::string> unknownAttrs;
                for (const auto &a : tagAttrs) {
                    if (settingValidAttrs.find(lowercase(a.first)) == settingValidAttrs.end()) {
                        unknownAttrs.push_back(a.first);
                    }
                }
                if (!unknownAttrs.empty()) {
                    std::sort(unknownAttrs.begin(), unknownAttrs.end(),
                              [](const std::string &a, const std::string &b) { return lowercase(a) < lowercase(b); });
                    std::string list;
                    for (size_t i = 0; i < unknownAttrs.size(); i++) {
                        if (i) list += ',';
                        list += uppercase(unknownAttrs[i]);
                    }
                    throw webstrada::exception(("Attribute validation error for tag CFSETTING. It does not allow the attribute(s) " + list + ". The valid attribute(s) are ENABLECFOUTPUTONLY,REQUESTTIMEOUT,SHOWDEBUGOUTPUT.").c_str());
                }
                llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                llvm::Value *outputVal = nullPtr, *debugVal = nullPtr, *timeoutVal = nullPtr;
                auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_DoubleString ||
                         valToks[0].token_id == TextParser_cfml_SingleString)) {
                        auto node = std::make_unique<ExprAST>();
                        node->type = ExprAST::LiteralString;
                        node->token = valToks[0];
                        return CompileExprAST(module, builder, mainfunc, node, cgi, server, cookie,
                                              application, session, url, form, variables, cfm_text);
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                for (const auto &a : tagAttrs) {
                    llvm::Value *val = compileValue(a.second);
                    std::string low = lowercase(a.first);
                    if (low == "enablecfoutputonly") outputVal = val;
                    else if (low == "showdebugoutput") debugVal = val;
                    else if (low == "requesttimeout") timeoutVal = val;
                }
                auto *fSetting = getOrCreateHelper(module, builder, "cf_setting", builder.getVoidTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                emitCall(builder, fSetting, {outputVal, debugVal, timeoutVal});
            } else if (tagNameLow.startWith("<cfimport") || tagNameLow.startWith("<cfmodule") ||
                       tagNameLow.startWith("<cfassociate")) {
                // <cfimport path="..."> registers a component import path that
                // CreateObject/`new` fall back to. The taglib/prefix forms and
                // <cfmodule>/<cfassociate> need the custom-tag runtime (not
                // implemented) and throw at runtime.
                const std::vector<TextParserTokenItem> *attrParts = &token.children;
                for (const auto &ch : token.children) {
                    if (ch.token_id == TextParser_cfml_Expression) {
                        attrParts = &ch.children;
                        break;
                    }
                }
                auto compileAttrValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                    if (valToks.empty()) return nullptr;
                    if (valToks.size() == 1 &&
                        (valToks[0].token_id == TextParser_cfml_Variable ||
                         valToks[0].token_id == TextParser_cfml_Number ||
                         valToks[0].token_id == TextParser_cfml_Boolean)) {
                        string raw(cfm_text + valToks[0].position, valToks[0].len);
                        auto *fStr = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                        return emitCall(builder, fStr,
                            {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                    }
                    auto ast = parseTokensToAST(valToks, cfm_text);
                    return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                          application, session, url, form, variables, cfm_text);
                };
                llvm::Value *aPath = nullptr, *aTaglib = nullptr, *aPrefix = nullptr;
                llvm::Value *aTemplate = nullptr, *aName = nullptr, *aArgColl = nullptr;
                llvm::Value *aBasetag = nullptr, *aDatacollection = nullptr;
                for (size_t ai = 0; ai < attrParts->size(); ) {
                    const auto &at = (*attrParts)[ai];
                    if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                    string aname(cfm_text + at.position, at.len);
                    string anameLow = aname;
                    anameLow.toLower();
                    std::vector<TextParserTokenItem> valToks;
                    size_t vi = ai + 1;
                    while (vi < attrParts->size()) {
                        const auto &vt = (*attrParts)[vi];
                        bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                           vi + 1 < attrParts->size() &&
                                           isOperatorToken((*attrParts)[vi + 1].token_id));
                        if (nextIsAttr) break;
                        if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                        vi++;
                    }
                    llvm::Value *val = valToks.empty() ? nullptr : compileAttrValue(valToks);
                    if (anameLow.equals("path")) aPath = val;
                    else if (anameLow.equals("taglib")) aTaglib = val;
                    else if (anameLow.equals("prefix")) aPrefix = val;
                    else if (anameLow.equals("template")) aTemplate = val;
                    else if (anameLow.equals("name")) aName = val;
                    else if (anameLow.equals("attributecollection")) aArgColl = val;
                    else if (anameLow.equals("basetag")) aBasetag = val;
                    else if (anameLow.equals("datacollection")) aDatacollection = val;
                    ai = vi;
                }
                auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
                if (tagNameLow.startWith("<cfimport")) {
                    if (aPath && (aTaglib || aPrefix)) {
                        throw webstrada::exception("cfimport",
                            "The path attribute cannot be combined with the taglib or prefix attributes.");
                    }
                    if (aTaglib || aPrefix) {
                        // JSP/custom-tag library import: requires the custom-tag
                        // runtime, not implemented.
                        auto *fTaglib = getOrCreateHelper(module, builder, "cf_import_taglib", builder.getVoidTy(),
                            {builder.getPtrTy(), builder.getPtrTy()});
                        emitCall(builder, fTaglib, {aTaglib ? aTaglib : nullPtr,
                                                    aPrefix ? aPrefix : nullPtr});
                    } else {
                        auto *fImport = getOrCreateHelper(module, builder, "cf_import_path", builder.getVoidTy(),
                            {builder.getPtrTy()});
                        emitCall(builder, fImport, {aPath ? aPath : nullPtr});
                    }
                } else if (tagNameLow.startWith("<cfmodule")) {
                    auto *fModule = getOrCreateHelper(module, builder, "cf_cfmodule", builder.getVoidTy(),
                        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
                    emitCall(builder, fModule, {aTemplate ? aTemplate : nullPtr,
                                                aName ? aName : nullPtr,
                                                aArgColl ? aArgColl : nullPtr});
                } else {
                    auto *fAssoc = getOrCreateHelper(module, builder, "cf_cfassociate", builder.getVoidTy(),
                        {builder.getPtrTy(), builder.getPtrTy()});
                    emitCall(builder, fAssoc, {aBasetag ? aBasetag : nullPtr,
                                               aDatacollection ? aDatacollection : nullPtr});
                }
            } else {
                throw webstrada::exception("compiler", "Unknown start tag: " + tokenName);
            }
            break;
        }

        case TextParser_cfml_OutputTagPair: {
            TextParserTokenItem outputStartTag, outputExpression, outputEndTag;
            bool hasOutputExpression = false;
            for (auto &child : token.children) {
                if (child.token_id == TextParser_cfml_OutputStartTag)
                    outputStartTag = child;
                else if (child.token_id == TextParser_cfml_OutputExpression) {
                    outputExpression = child;
                    hasOutputExpression = true;
                } else if (child.token_id == TextParser_cfml_OutputEndTag)
                    outputEndTag = child;
            }

            if (hasOutputExpression) {
                validateOutputExpressionSharp(outputExpression, cfm_text);
            }

            auto outTagText = string(cfm_text + outputStartTag.position, outputStartTag.len);
            auto outAttrs = parse_attributes(outTagText);

            string outQueryExpr = outAttrs.count("query") ? outAttrs["query"] : "";
            string outStartRowExpr = outAttrs.count("startrow") ? outAttrs["startrow"] : "";
            string outMaxRowsExpr = outAttrs.count("maxrows") ? outAttrs["maxrows"] : "";
            string outGroupCol = outAttrs.count("group") ? outAttrs["group"] : "";
            string outGroupCs = outAttrs.count("groupcasesensitive") ? outAttrs["groupcasesensitive"] : "";

            auto compileAttrExpr = [&](const string &e) -> llvm::Value* {
                string ex = e.trimmed();
                if (ex.length() >= 2 && ex.first() == '#' && ex.at(ex.length() - 1) == '#')
                    ex = ex.mid(1, ex.length() - 2).trimmed();
                return CompileStringExpression(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, ex, cfm_text);
            };

            size_t startTagEnd = outputStartTag.position + outputStartTag.len;

            // ColdFusion also applies whitespace management to whitespace-only
            // text inside a <cfoutput> body when it sits next to a #...# Sharp
            // expression (e.g. a trailing line break after #expr# collapses to a
            // single space, and leading whitespace before the first #expr# is
            // removed). Plain text runs are emitted verbatim, so management is
            // enabled with the <cfoutput> start tag as the left neighbour.
            // Inside <cfoutput> the text writes bypass the cfoutputonly gate:
            // <cfsetting enablecfoutputonly> only suppresses content outside
            // cfoutput tags.
            ScopedCodegenState<bool> insideCfoutputGuard(g_insideCfoutput, true);

            if (outQueryExpr.isEmpty()) {
                WhitespaceState wsOutput(ws.enabled, ws.flag);
                wsOutput.markTag(false, false); // left neighbour is the <cfoutput> start tag

                if (!hasOutputExpression) {
                    wsOutput.feed(module, builder, out, cfm_text + startTagEnd, outputEndTag.position - startTagEnd, WsRight::Tag);
                    wsOutput.finish(module, builder, out, WsRight::Tag);
                } else {
                    if (outputExpression.position > startTagEnd) {
                        wsOutput.feed(module, builder, out, cfm_text + startTagEnd, outputExpression.position - startTagEnd, WsRight::Tag);
                    }

                    size_t exprPos = outputExpression.position;
                    size_t oidx = 0;
                    compile_token_list(outputExpression.children, oidx, exprPos, context, module, builder, mainfunc,
                                       out, wsOutput, cgi, server, cookie, application, session, url, form, variables,
                                       cfm_text, outputExpression.position + outputExpression.len, loopStack);

                    size_t exprEnd = outputExpression.position + outputExpression.len;
                    if (exprPos < exprEnd) {
                        wsOutput.feed(module, builder, out, cfm_text + exprPos, exprEnd - exprPos, WsRight::Tag);
                    }
                    wsOutput.finish(module, builder, out, WsRight::Tag);
                }
            } else {
                // <cfoutput query="q" startrow=".." maxrows=".." group=".."
                // groupcasesensitive=".."> iterates the query's rows (each
                // group's first row when `group` is set), advancing the query's
                // cursor so #q.col# / #col# and unqualified column names resolve
                // to the current row.
                auto *fRowcount = getOrCreateHelper(module, builder, "cf_query_rowcount", builder.getInt64Ty(), {builder.getPtrTy()});
                auto *fSetRow = getOrCreateHelper(module, builder, "cf_query_set_row", builder.getVoidTy(), {builder.getPtrTy(), builder.getInt64Ty()});
                auto *fScopePush = getOrCreateHelper(module, builder, "cf_query_scope_push", builder.getInt64Ty(), {builder.getPtrTy()});
                auto *fScopePop = getOrCreateHelper(module, builder, "cf_query_scope_pop", builder.getVoidTy(), {});
                auto *fToLong = getOrCreateHelper(module, builder, "cfvariant_to_long", builder.getInt64Ty(), {builder.getPtrTy()});
                auto *fResolve = getOrCreateHelper(module, builder, "cf_query_resolve", builder.getPtrTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy()});

                llvm::Value *rawQueryVal = compileAttrExpr(outQueryExpr);
                llvm::AllocaInst *lpQuerySlot = createEntryAlloca(builder, mainfunc, builder.getPtrTy());
                builder.CreateStore(emitCall(builder, fResolve, {rawQueryVal, cgi, server, cookie, application,
                                                                 session, url, form, variables}), lpQuerySlot);
                llvm::Value *collVal = builder.CreateLoad(builder.getPtrTy(), lpQuerySlot);

                llvm::Value *startRowVal;
                if (outStartRowExpr.isEmpty()) {
                    startRowVal = builder.getInt64(1);
                } else {
                    startRowVal = emitCall(builder, fToLong, {compileAttrExpr(outStartRowExpr)});
                }

                llvm::Value *endRowVal;
                if (outMaxRowsExpr.isEmpty()) {
                    endRowVal = emitCall(builder, fRowcount, {collVal});
                } else {
                    llvm::Value *maxRowsVal = emitCall(builder, fToLong, {compileAttrExpr(outMaxRowsExpr)});
                    // endRow = startRow + maxRows - 1
                    llvm::Value *sum = builder.CreateAdd(startRowVal, maxRowsVal);
                    endRowVal = builder.CreateSub(sum, builder.getInt64(1));
                }

                llvm::AllocaInst *lpIndexVar = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
                llvm::AllocaInst *lpLenVar = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
                builder.CreateStore(endRowVal, lpLenVar);

                auto *rowCountVal = emitCall(builder, fRowcount, {collVal});
                llvm::AllocaInst *lpCollVal = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
                builder.CreateStore(rowCountVal, lpCollVal);

                llvm::AllocaInst *lpSaveRowSlot = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
                builder.CreateStore(emitCall(builder, fScopePush, {collVal}), lpSaveRowSlot);

                builder.CreateStore(startRowVal, lpIndexVar);
                emitCall(builder, fSetRow, {collVal, startRowVal});

                auto lpIncBB = llvm::BasicBlock::Create(context, "cfoutput.inc", mainfunc);
                auto lpCondBB = llvm::BasicBlock::Create(context, "cfoutput.cond", mainfunc);
                auto lpBodyBB = llvm::BasicBlock::Create(context, "cfoutput.body", mainfunc);
                auto lpEndBB = llvm::BasicBlock::Create(context, "cfoutput.end", mainfunc);

                builder.CreateBr(lpCondBB);
                builder.SetInsertPoint(lpCondBB);
                auto curIdx = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                auto curEnd = builder.CreateLoad(builder.getInt64Ty(), lpLenVar);
                auto curRowCount = builder.CreateLoad(builder.getInt64Ty(), lpCollVal);
                auto pastEnd = builder.CreateICmpSGT(curIdx, curEnd);
                auto pastCount = builder.CreateICmpSGT(curIdx, curRowCount);
                auto done = builder.CreateOr(pastEnd, pastCount);
                builder.CreateCondBr(done, lpEndBB, lpBodyBB);

                builder.SetInsertPoint(lpBodyBB);

                loopStack.push_back({lpIncBB, lpEndBB, "", lpIndexVar});

                WhitespaceState wsOutput(ws.enabled, ws.flag);
                wsOutput.markTag(false, false);

                if (!hasOutputExpression) {
                    wsOutput.feed(module, builder, out, cfm_text + startTagEnd, outputEndTag.position - startTagEnd, WsRight::Tag);
                    wsOutput.finish(module, builder, out, WsRight::Tag);
                } else {
                    if (outputExpression.position > startTagEnd) {
                        wsOutput.feed(module, builder, out, cfm_text + startTagEnd, outputExpression.position - startTagEnd, WsRight::Tag);
                    }

                    size_t exprPos = outputExpression.position;
                    size_t oidx = 0;
                    compile_token_list(outputExpression.children, oidx, exprPos, context, module, builder, mainfunc,
                                       out, wsOutput, cgi, server, cookie, application, session, url, form, variables,
                                       cfm_text, outputExpression.position + outputExpression.len, loopStack);

                    size_t exprEnd = outputExpression.position + outputExpression.len;
                    if (exprPos < exprEnd) {
                        wsOutput.feed(module, builder, out, cfm_text + exprPos, exprEnd - exprPos, WsRight::Tag);
                    }
                    wsOutput.finish(module, builder, out, WsRight::Tag);
                }

                loopStack.pop_back();

                if (!builder.GetInsertBlock()->getTerminator()) {
                    builder.CreateBr(lpIncBB);
                }
                builder.SetInsertPoint(lpIncBB);

                llvm::Value *nextRow;
                if (!outGroupCol.isEmpty()) {
                    auto *fGroupNext = getOrCreateHelper(module, builder, "cf_query_group_next", builder.getInt64Ty(),
                        {builder.getPtrTy(), builder.getPtrTy(), builder.getInt64Ty(), builder.getInt64Ty(), builder.getPtrTy()});
                    auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                    auto *groupColVal = emitCall(builder, fCreateString,
                        {builder.CreateGlobalString(llvm::StringRef(outGroupCol.constData(), outGroupCol.length()), "", 0, module, true)});
                    llvm::Value *caseSensVal = llvm::ConstantPointerNull::get(builder.getPtrTy());
                    if (!outGroupCs.isEmpty()) {
                        caseSensVal = emitCall(builder, fCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(outGroupCs.constData(), outGroupCs.length()), "", 0, module, true)});
                    }
                    auto curRowG = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                    auto curEndG = builder.CreateLoad(builder.getInt64Ty(), lpLenVar);
                    nextRow = emitCall(builder, fGroupNext, {collVal, groupColVal, curRowG, curEndG, caseSensVal});
                } else {
                    auto incIdx4 = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                    nextRow = builder.CreateAdd(incIdx4, builder.getInt64(1));
                }
                builder.CreateStore(nextRow, lpIndexVar);
                emitCall(builder, fSetRow, {collVal, nextRow});
                builder.CreateBr(lpCondBB);

                builder.SetInsertPoint(lpEndBB);
                auto origRow = builder.CreateLoad(builder.getInt64Ty(), lpSaveRowSlot);
                emitCall(builder, fSetRow, {collVal, origRow});
                emitCall(builder, fScopePop, {});
            }
            break;
        }



        case TextParser_cfml_ScriptTagPair: {
            TextParserTokenItem scriptStartTag, scriptExpression, scriptEndTag;
            bool hasExpr = false;
            for (auto &child : token.children) {
                if (child.token_id == TextParser_cfml_ScriptStartTag) {
                    scriptStartTag = child;
                } else if (child.token_id == TextParser_cfml_ScriptExpression) {
                    scriptExpression = child;
                    hasExpr = true;
                } else if (child.token_id == TextParser_cfml_ScriptEndTag) {
                    scriptEndTag = child;
                }
            }

            if (hasExpr) {
                compile_script_expression(scriptExpression.children, context, module, builder, mainfunc,
                                          out, cgi, server, cookie, application, session, url, form, variables,
                                          cfm_text, scriptExpression.position + scriptExpression.len, loopStack);
            }
            break;
        }

        case TextParser_cfml_LoopTagPair: {
            TextParserTokenItem lpStart, lpExpression, lpEnd;
            bool hasExpr = false;
            for (auto &child : token.children) {
                if (child.token_id == TextParser_cfml_LoopStartTag) lpStart = child;
                else if (child.token_id == TextParser_cfml_LoopExpression) { lpExpression = child; hasExpr = true; }
                else if (child.token_id == TextParser_cfml_LoopEndTag) lpEnd = child;
            }

            auto lpTagText = string(cfm_text + lpStart.position, lpStart.len);
            auto lpAttrs = parse_attributes(lpTagText);

            // Compiles a `<cfloop>` numeric bound (from/to/step) the way CF
            // does: the attribute value is a CFML-interpolated string — a
            // numeric literal, or any mix of literal text and `#expr#`
            // interpolations (`to="#x##y#"` -> "57", `to="#x#5"` -> "55") —
            // which is then cast to a long. A value with `#` is tokenized as a
            // quoted literal so the existing LiteralString interpolation path
            // evaluates each `#...#` group. A bare non-numeric value with no
            // `#` (e.g. `to="1 + 2"`) is a compile error, matching CF.
            auto compileInterpBound = [&](const string &raw) -> llvm::Value* {
                string ex = raw.trimmed();
                if (ex.isEmpty()) {
                    auto *f = module->getFunction("cfvariant_create_string");
                    if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                    return emitCall(builder, f, {builder.CreateGlobalString(llvm::StringRef("", 0), "", 0, module, true)});
                }
                if (ex.indexOf('#') < 0) {
                    // No interpolation: CF accepts only a numeric literal here
                    // (integer or float, whitespace-trimmed); anything else
                    // (e.g. `1 + 2`) is a compile error.
                    char *ep = nullptr;
                    strtod(ex.constData(), &ep);
                    if (ep == ex.constData() || *ep != '\0') {
                        webstrada::string msg("Invalid attribute value: ");
                        msg.append(ex);
                        throw webstrada::exception("cfloop", msg);
                    }
                    auto *f = module->getFunction("cfvariant_create_string");
                    if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                    return emitCall(builder, f, {builder.CreateGlobalString(llvm::StringRef(ex.constData(), ex.length()), "", 0, module, true)});
                }
                // Contains `#`: tokenize `<cfset __x = "<value>">` and compile
                // the DoubleString as a LiteralString so `#expr#` interpolates
                // each group and literal text is concatenated.
                string wrapped = "<cfset __x = \"" + ex + "\">";
                parser p;
                p.parse(wrapped.constData(), wrapped.length(), TEXTPARSER_ENCODING_LATIN1);
                std::vector<TextParserTokenItem> toks;
                while (auto t = p.next_token()) toks.push_back(convertToken(t));
                for (auto &t0 : toks) {
                    if (t0.token_id != TextParser_cfml_StartTag) continue;
                    for (auto &child : t0.children) {
                        if (child.token_id != TextParser_cfml_Expression) continue;
                        for (auto &exprChild : child.children) {
                            if (exprChild.token_id == TextParser_cfml_DoubleString) {
                                auto node = std::make_unique<ExprAST>();
                                node->type = ExprAST::LiteralString;
                                node->token = exprChild;
                                return CompileExprAST(module, builder, mainfunc, node, cgi, server, cookie,
                                                      application, session, url, form, variables, p.get_text());
                            }
                        }
                    }
                }
                // Fall back to a plain string value.
                auto *f = module->getFunction("cfvariant_create_string");
                if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                return emitCall(builder, f, {builder.CreateGlobalString(llvm::StringRef(ex.constData(), ex.length()), "", 0, module, true)});
            };

            string lpIndexName = lpAttrs.count("index") ? lpAttrs["index"] : "";
            string lpItemName = lpAttrs.count("item") ? lpAttrs["item"] : "";
            string lpFromExpr = lpAttrs.count("from") ? lpAttrs["from"] : "";
            string lpToExpr = lpAttrs.count("to") ? lpAttrs["to"] : "";
            string lpStepExpr = lpAttrs.count("step") ? lpAttrs["step"] : "1";
            string lpListExpr = lpAttrs.count("list") ? lpAttrs["list"] : "";
            string lpDels = lpAttrs.count("delimiters") ? lpAttrs["delimiters"] : ",";
            string lpArrayExpr = lpAttrs.count("array") ? lpAttrs["array"] : "";
            string lpCollExpr = lpAttrs.count("collection") ? lpAttrs["collection"] : "";
            string lpCondExpr = lpAttrs.count("condition") ? lpAttrs["condition"] : "";
            string lpQueryExpr = lpAttrs.count("query") ? lpAttrs["query"] : "";
            string lpStartRowExpr = lpAttrs.count("startrow") ? lpAttrs["startrow"] : "";
            string lpEndRowExpr = lpAttrs.count("endrow") ? lpAttrs["endrow"] : "";
            string lpGroupCol = lpAttrs.count("group") ? lpAttrs["group"] : "";
            string lpGroupCs = lpAttrs.count("groupcasesensitive") ? lpAttrs["groupcasesensitive"] : "";

            enum { LOOP_NUMERIC, LOOP_LIST, LOOP_ARRAY, LOOP_COLLECTION, LOOP_CONDITION, LOOP_QUERY } loopKind;
            if (!lpQueryExpr.isEmpty()) loopKind = LOOP_QUERY;
            else if (!lpCondExpr.isEmpty()) loopKind = LOOP_CONDITION;
            else if (!lpArrayExpr.isEmpty()) loopKind = LOOP_ARRAY;
            else if (!lpCollExpr.isEmpty()) loopKind = LOOP_COLLECTION;
            else if (!lpListExpr.isEmpty()) loopKind = LOOP_LIST;
            else loopKind = LOOP_NUMERIC;

            if (loopKind == LOOP_NUMERIC && (lpIndexName.isEmpty() || lpFromExpr.isEmpty() || lpToExpr.isEmpty()))
                throw webstrada::exception("cfloop", "Missing required attributes (from, to, index)");
            if ((loopKind == LOOP_LIST || loopKind == LOOP_ARRAY) && lpIndexName.isEmpty())
                throw webstrada::exception("cfloop", "cfloop list/array requires an index attribute");
            if (loopKind == LOOP_COLLECTION && lpItemName.isEmpty())
                throw webstrada::exception("cfloop", "cfloop collection requires an item attribute");

            auto lpIncBB = llvm::BasicBlock::Create(context, "loop.inc", mainfunc);
            auto lpCondBB = llvm::BasicBlock::Create(context, "loop.cond", mainfunc);
            auto lpBodyBB = llvm::BasicBlock::Create(context, "loop.body", mainfunc);
            auto lpEndBB = llvm::BasicBlock::Create(context, "loop.end", mainfunc);

            auto lpSetFunc = module->getFunction("cfloop_set_long");
            if (!lpSetFunc) {
                std::vector<llvm::Type*> sp(10, builder.getPtrTy());
                sp[9] = builder.getInt64Ty();
                lpSetFunc = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), sp, false),
                    llvm::Function::InternalLinkage, "cfloop_set_long", module);
            }

            auto getLPIntExpr = [&](const string &e) -> llvm::Value* {
                string ex = e.trimmed();
                // A single #...#-wrapped expression (e.g. `to="#x + y#"`,
                // `to="#5+2#"`) evaluates the inner expression directly, like
                // CF — faster than going through string interpolation.
                if (ex.length() >= 2 && ex.first() == '#' && ex.at(ex.length() - 1) == '#') {
                    bool innerHasSharp = ex.indexOf('#', 1) >= 0 && ex.indexOf('#', 1) < ex.length() - 1;
                    if (!innerHasSharp) {
                        string inner = ex.mid(1, ex.length() - 2).trimmed();
                        auto *val = CompileStringExpression(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, inner, cfm_text);
                        auto *f = module->getFunction("cfvariant_to_long");
                        if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getInt64Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_to_long", module);
                        return emitCall(builder, f, {val});
                    }
                }
                // Otherwise interpolate literal text + #expr# groups and cast.
                llvm::Value *val = compileInterpBound(ex);
                auto *f = module->getFunction("cfvariant_to_long");
                if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getInt64Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_to_long", module);
                return emitCall(builder, f, {val});
            };

            // Compiles an attribute expression, unwrapping the surrounding #...#.
            auto compileAttrExpr = [&](const string &e) -> llvm::Value* {
                string ex = e.trimmed();
                if (ex.length() >= 2 && ex.first() == '#' && ex.at(ex.length() - 1) == '#')
                    ex = ex.mid(1, ex.length() - 2).trimmed();
                return CompileStringExpression(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, ex, cfm_text);
            };

            // Loop variable set each iteration: numeric/list/array use `index`,
            // collection uses `item`.
            string lpLoopVarName = (loopKind == LOOP_COLLECTION) ? lpItemName : lpIndexName;

llvm::AllocaInst *lpIndexVar = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
            llvm::AllocaInst *lpLenVar = nullptr;
            llvm::AllocaInst *lpStepVar = nullptr;
            llvm::AllocaInst *lpCollVal = nullptr;
            llvm::AllocaInst *lpQuerySlot = nullptr;
            llvm::Value *collVal = nullptr;
            llvm::Value *delimsVal = nullptr;

            if (loopKind == LOOP_NUMERIC) {
                auto lpToVar = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
                lpStepVar = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
                auto lpFromVal = getLPIntExpr(lpFromExpr);
                auto lpToVal = getLPIntExpr(lpToExpr);
                auto lpStepVal = getLPIntExpr(lpStepExpr);
                builder.CreateStore(lpFromVal, lpIndexVar);
                builder.CreateStore(lpToVal, lpToVar);
                builder.CreateStore(lpStepVal, lpStepVar);
                {
                    auto g = builder.CreateGlobalString(llvm::StringRef(lpIndexName.constData(), lpIndexName.length()), "", 0, module, true);
                    llvm::Value *sa[] = {cgi, server, cookie, application, session, url, form, variables, g, lpFromVal};
                    emitCall(builder, lpSetFunc, sa);
                }

                builder.CreateBr(lpCondBB);
                builder.SetInsertPoint(lpCondBB);
                auto curIdx = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                auto curTo = builder.CreateLoad(builder.getInt64Ty(), lpToVar);
                auto curStep = builder.CreateLoad(builder.getInt64Ty(), lpStepVar);
                auto stepGt0 = builder.CreateICmpSGT(curStep, builder.getInt64(0));
                auto idxLeTo = builder.CreateICmpSLE(curIdx, curTo);
                auto fwd = builder.CreateAnd(stepGt0, idxLeTo);
                auto stepLt0 = builder.CreateICmpSLT(curStep, builder.getInt64(0));
                auto idxGeTo = builder.CreateICmpSGE(curIdx, curTo);
                auto rev = builder.CreateAnd(stepLt0, idxGeTo);
                auto lpCond = builder.CreateOr(fwd, rev);
                builder.CreateCondBr(lpCond, lpBodyBB, lpEndBB);
            } else if (loopKind == LOOP_LIST || loopKind == LOOP_ARRAY || loopKind == LOOP_COLLECTION) {
                if (loopKind == LOOP_LIST) {
                    string lv = lpListExpr.trimmed();
                    if (lv.length() >= 2 && lv.first() == '#' && lv.at(lv.length() - 1) == '#') {
                        collVal = compileAttrExpr(lpListExpr);
                    } else {
                        auto *fStr = module->getFunction("cfvariant_create_string");
                        if (!fStr) fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                        collVal = emitCall(builder, fStr, {builder.CreateGlobalString(llvm::StringRef(lv.constData(), lv.length()), "", 0, module, true)});
                    }
                } else if (loopKind == LOOP_ARRAY) {
                    collVal = compileAttrExpr(lpArrayExpr);
                } else {
                    collVal = compileAttrExpr(lpCollExpr);
                }

                auto *fStr = module->getFunction("cfvariant_create_string");
                if (!fStr) fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                delimsVal = emitCall(builder, fStr, {builder.CreateGlobalString(llvm::StringRef(lpDels.constData(), lpDels.length()), "", 0, module, true)});

                auto *fLen = module->getFunction("cfforin_length");
                if (!fLen) fLen = llvm::Function::Create(llvm::FunctionType::get(builder.getInt64Ty(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfforin_length", module);
                lpLenVar = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
                builder.CreateStore(emitCall(builder, fLen, {collVal, delimsVal}), lpLenVar);
                builder.CreateStore(builder.getInt64(1), lpIndexVar);

                builder.CreateBr(lpCondBB);
                builder.SetInsertPoint(lpCondBB);
                auto curIdx2 = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                auto curLen = builder.CreateLoad(builder.getInt64Ty(), lpLenVar);
                auto done = builder.CreateICmpSGT(curIdx2, curLen);
                builder.CreateCondBr(done, lpEndBB, lpBodyBB);
            } else if (loopKind == LOOP_QUERY) {
                // <cfloop query="q" startrow=".." endrow=".." group=".."
                // groupcasesensitive=".."> iterates the query's rows (each
                // group's first row when `group` is set), advancing the query's
                // cursor so #q.col# / #q.currentrow# and unqualified column
                // names resolve to the current row. The query scope is pushed
                // for the whole loop (CF's pushQueryScope) and popped at the
                // end; the cursor is restored to its original row.
                auto *fRowcount = getOrCreateHelper(module, builder, "cf_query_rowcount", builder.getInt64Ty(), {builder.getPtrTy()});
                auto *fSetRow = getOrCreateHelper(module, builder, "cf_query_set_row", builder.getVoidTy(), {builder.getPtrTy(), builder.getInt64Ty()});
                auto *fScopePush = getOrCreateHelper(module, builder, "cf_query_scope_push", builder.getInt64Ty(), {builder.getPtrTy()});
                auto *fScopePop = getOrCreateHelper(module, builder, "cf_query_scope_pop", builder.getVoidTy(), {});
                auto *fToLong = getOrCreateHelper(module, builder, "cfvariant_to_long", builder.getInt64Ty(), {builder.getPtrTy()});
                auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                auto *fResolve = getOrCreateHelper(module, builder, "cf_query_resolve", builder.getPtrTy(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                     builder.getPtrTy(), builder.getPtrTy()});

                // The query attribute may be a literal name, an interpolated
                // string (query="#qname#") or a live query value; resolve it
                // once and reuse the resolved pointer for the whole loop.
                llvm::Value *rawQueryVal = compileAttrExpr(lpQueryExpr);
                lpQuerySlot = createEntryAlloca(builder, mainfunc, builder.getPtrTy());
                builder.CreateStore(emitCall(builder, fResolve, {rawQueryVal, cgi, server, cookie, application,
                                                                 session, url, form, variables}), lpQuerySlot);
                collVal = builder.CreateLoad(builder.getPtrTy(), lpQuerySlot);

                llvm::Value *startRowVal;
                if (lpStartRowExpr.isEmpty()) {
                    startRowVal = builder.getInt64(1);
                } else {
                    startRowVal = emitCall(builder, fToLong, {compileAttrExpr(lpStartRowExpr)});
                }
                llvm::Value *endRowVal;
                if (lpEndRowExpr.isEmpty()) {
                    endRowVal = emitCall(builder, fRowcount, {collVal});
                } else {
                    endRowVal = emitCall(builder, fToLong, {compileAttrExpr(lpEndRowExpr)});
                }

                lpLenVar = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
                builder.CreateStore(endRowVal, lpLenVar);

                // The loop is bounded by the actual row count too (CF stops at
                // the last row even when endrow exceeds it, and a startrow past
                // the last row skips the body entirely).
                auto *rowCountVal = emitCall(builder, fRowcount, {collVal});
                lpCollVal = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
                builder.CreateStore(rowCountVal, lpCollVal);

                // Push the query scope; remember the original cursor to restore
                // when the loop exits (lpStepVar doubles as the save slot).
                lpStepVar = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
                builder.CreateStore(emitCall(builder, fScopePush, {collVal}), lpStepVar);

                // Cursor starts at startrow.
                builder.CreateStore(startRowVal, lpIndexVar);
                emitCall(builder, fSetRow, {collVal, startRowVal});

                builder.CreateBr(lpCondBB);
                builder.SetInsertPoint(lpCondBB);
                auto curIdx = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                auto curEnd = builder.CreateLoad(builder.getInt64Ty(), lpLenVar);
                auto curRowCount = builder.CreateLoad(builder.getInt64Ty(), lpCollVal);
                auto pastEnd = builder.CreateICmpSGT(curIdx, curEnd);
                auto pastCount = builder.CreateICmpSGT(curIdx, curRowCount);
                auto done = builder.CreateOr(pastEnd, pastCount);
                builder.CreateCondBr(done, lpEndBB, lpBodyBB);
            } else { // LOOP_CONDITION
                builder.CreateBr(lpCondBB);
                builder.SetInsertPoint(lpCondBB);
                auto condVal = compileAttrExpr(lpCondExpr);
                auto *fTruthy = module->getFunction("cfvariant_is_truthy");
                if (!fTruthy) fTruthy = llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_is_truthy", module);
                auto condBool = builder.CreateICmpNE(emitCall(builder, fTruthy, {condVal}), builder.getInt32(0));
                builder.CreateCondBr(condBool, lpBodyBB, lpEndBB);
            }

            builder.SetInsertPoint(lpBodyBB);

            // Set the loop variable before each iteration for the iteration
            // kinds (list/array/collection). The index is assigned like an
            // unqualified <cfset>: inside a function a `var`-declared loop
            // variable lands in the local scope (in a component method the
            // `variables` scope is the instance scope, NOT the function-local
            // scope, so writing it there would leave the loop variable empty).
            if (loopKind == LOOP_LIST || loopKind == LOOP_ARRAY || loopKind == LOOP_COLLECTION) {
                auto *fItem = module->getFunction("cfforin_item");
                if (!fItem) fItem = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getInt64Ty(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfforin_item", module);
                auto *fAssignIndex = module->getFunction("cfloop_assign_index");
                if (!fAssignIndex) fAssignIndex = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), std::vector<llvm::Type*>(10, builder.getPtrTy()), false), llvm::Function::InternalLinkage, "cfloop_assign_index", module);
                auto *curIdx3 = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                auto *itemVal = emitCall(builder, fItem, {collVal, curIdx3, delimsVal});
                auto *varName = builder.CreateGlobalString(llvm::StringRef(lpLoopVarName.constData(), lpLoopVarName.length()), "", 0, module, true);
                emitCall(builder, fAssignIndex, {cgi, server, cookie, application, session, url, form, variables, varName, itemVal});
            }

            string lpIndexUpper = lpLoopVarName;
            lpIndexUpper.toUpper();
            loopStack.push_back({lpIncBB, lpEndBB, lpIndexUpper, lpIndexVar});

            // The whole loop body is the raw source range between the end of the
            // <cfloop> start tag and the start of </cfloop>. The textparser only
            // creates a LoopExpression child when the body contains a nested CFML
            // token, and that group starts at its first child token, so leading
            // raw text (and a fully raw-text body) is not covered by any token.
            // Recover it directly from the source so it is emitted verbatim.
            size_t bodyStart = lpStart.position + lpStart.len;
            size_t bodyEnd = lpEnd.position;

            WhitespaceState wsLoop(ws.enabled, ws.flag);
            wsLoop.markTag(false, false); // left neighbour is <cfloop>

            if (hasExpr) {
                size_t bodyPos = bodyStart;
                size_t lidx = 0;
                compile_token_list(lpExpression.children, lidx, bodyPos, context, module, builder, mainfunc,
                                   out, wsLoop, cgi, server, cookie, application, session, url, form, variables,
                                   cfm_text, bodyEnd, loopStack);
                if (bodyPos < bodyEnd) {
                    wsLoop.feed(module, builder, out, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
                }
            } else {
                wsLoop.feed(module, builder, out, cfm_text + bodyStart, bodyEnd - bodyStart, WsRight::Tag);
            }
            wsLoop.finish(module, builder, out, WsRight::Tag);

            loopStack.pop_back();

            if (loopKind == LOOP_NUMERIC) {
                builder.CreateBr(lpIncBB);
                builder.SetInsertPoint(lpIncBB);
                auto incIdx2 = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                auto incStep2 = builder.CreateLoad(builder.getInt64Ty(), lpStepVar);
                auto newIdx2 = builder.CreateAdd(incIdx2, incStep2);
                builder.CreateStore(newIdx2, lpIndexVar);
                {
                    auto g = builder.CreateGlobalString(llvm::StringRef(lpIndexName.constData(), lpIndexName.length()), "", 0, module, true);
                    llvm::Value *sa2[] = {cgi, server, cookie, application, session, url, form, variables, g, newIdx2};
                    emitCall(builder, lpSetFunc, sa2);
                }
                builder.CreateBr(lpCondBB);
            } else if (loopKind == LOOP_LIST || loopKind == LOOP_ARRAY || loopKind == LOOP_COLLECTION) {
                if (!builder.GetInsertBlock()->getTerminator()) {
                    builder.CreateBr(lpIncBB);
                }
                builder.SetInsertPoint(lpIncBB);
                auto incIdx3 = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                builder.CreateStore(builder.CreateAdd(incIdx3, builder.getInt64(1)), lpIndexVar);
                builder.CreateBr(lpCondBB);
            } else if (loopKind == LOOP_QUERY) {
                if (!builder.GetInsertBlock()->getTerminator()) {
                    builder.CreateBr(lpIncBB);
                }
                builder.SetInsertPoint(lpIncBB);
                // Advance to the next row — or, with `group`, the first row of
                // the next group (CF runs each group's first row) — and move the
                // query cursor there.
                llvm::Value *nextRow;
                if (!lpGroupCol.isEmpty()) {
                    auto *fGroupNext = getOrCreateHelper(module, builder, "cf_query_group_next", builder.getInt64Ty(),
                        {builder.getPtrTy(), builder.getPtrTy(), builder.getInt64Ty(), builder.getInt64Ty(), builder.getPtrTy()});
                    auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
                    auto *groupColVal = emitCall(builder, fCreateString,
                        {builder.CreateGlobalString(llvm::StringRef(lpGroupCol.constData(), lpGroupCol.length()), "", 0, module, true)});
                    llvm::Value *caseSensVal = llvm::ConstantPointerNull::get(builder.getPtrTy());
                    if (!lpGroupCs.isEmpty()) {
                        caseSensVal = emitCall(builder, fCreateString,
                            {builder.CreateGlobalString(llvm::StringRef(lpGroupCs.constData(), lpGroupCs.length()), "", 0, module, true)});
                    }
                    auto curRowG = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                    auto curEndG = builder.CreateLoad(builder.getInt64Ty(), lpLenVar);
                    nextRow = emitCall(builder, fGroupNext, {collVal, groupColVal, curRowG, curEndG, caseSensVal});
                } else {
                    auto incIdx4 = builder.CreateLoad(builder.getInt64Ty(), lpIndexVar);
                    nextRow = builder.CreateAdd(incIdx4, builder.getInt64(1));
                }
                builder.CreateStore(nextRow, lpIndexVar);
                auto *fSetRow = getOrCreateHelper(module, builder, "cf_query_set_row", builder.getVoidTy(), {builder.getPtrTy(), builder.getInt64Ty()});
                emitCall(builder, fSetRow, {collVal, nextRow});
                builder.CreateBr(lpCondBB);
            } else { // LOOP_CONDITION
                if (!builder.GetInsertBlock()->getTerminator()) {
                    builder.CreateBr(lpCondBB);
                }
                builder.SetInsertPoint(lpEndBB);
            }

            if (loopKind == LOOP_QUERY) {
                // Loop exit: restore the query's original cursor and pop the
                // query scope (cfbreak lands here too).
                builder.SetInsertPoint(lpEndBB);
                auto *fSetRow = getOrCreateHelper(module, builder, "cf_query_set_row", builder.getVoidTy(), {builder.getPtrTy(), builder.getInt64Ty()});
                auto *fScopePop = getOrCreateHelper(module, builder, "cf_query_scope_pop", builder.getVoidTy(), {});
                auto origRow = builder.CreateLoad(builder.getInt64Ty(), lpStepVar);
                emitCall(builder, fSetRow, {collVal, origRow});
                emitCall(builder, fScopePop, {});
            }

            if (loopKind != LOOP_CONDITION && loopKind != LOOP_QUERY) {
                builder.SetInsertPoint(lpEndBB);
            }
            break;
        }

        case TextParser_cfml_QueryTagPair: {
            // `<cfquery>` executes the (evaluated) SQL in its body against the
            // datasource's SQLite database and stores the result in the `name`
            // variable plus the `result` metadata struct. Attribute values are
            // compiled like <cfimage>/<cfthrow>, so `maxrows="#n#"` evaluates;
            // the body is captured into a runtime buffer (cf_query_begin) with
            // its #...# expressions evaluated, then handed to cf_query_end.
            TextParserTokenItem qpStart, qpExpression, qpEnd;
            bool hasExpr = false;
            for (auto &child : token.children) {
                if (child.token_id == TextParser_cfml_QueryStartTag) qpStart = child;
                else if (child.token_id == TextParser_cfml_QueryExpression) { qpExpression = child; hasExpr = true; }
                else if (child.token_id == TextParser_cfml_QueryEndTag) qpEnd = child;
            }
            if (qpStart.len == 0) {
                throw webstrada::exception("cfquery", "Missing <cfquery> start tag");
            }

            const std::vector<TextParserTokenItem> *attrParts = &qpStart.children;
            for (const auto &ch : qpStart.children) {
                if (ch.token_id == TextParser_cfml_Expression) {
                    attrParts = &ch.children;
                    break;
                }
            }

            auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
            auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                                   {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
            auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
            auto *fQueryBegin = getOrCreateHelper(module, builder, "cf_query_begin", builder.getPtrTy(), {});
            auto *fQueryEnd = getOrCreateHelper(module, builder, "cf_query_end", builder.getPtrTy(),
                {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                 builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                 builder.getPtrTy(), builder.getPtrTy()});

            // The valid attribute set (CF's QueryTag bean); unknown attributes
            // are compile-time errors like CF. Basic ones are implemented by
            // cf_query_end; the rest are accepted (some are ignored).
            static const std::unordered_set<std::string> validAttrs = {
                "name", "datasource", "timezone", "dbtype", "username", "password",
                "maxrows", "blockfactor", "timeout", "cachedafter", "cachedwithin",
                "debug", "result", "ormoptions", "cacheid", "cacheregion",
                "clientinfo", "fetchclientinfo", "lazy", "psq", "returntype", "columnkey"
            };

            auto compileValue = [&](const std::vector<TextParserTokenItem> &valToks) -> llvm::Value* {
                if (valToks.empty()) return nullptr;
                if (valToks.size() == 1 &&
                    (valToks[0].token_id == TextParser_cfml_Variable ||
                     valToks[0].token_id == TextParser_cfml_Number ||
                     valToks[0].token_id == TextParser_cfml_Boolean)) {
                    string raw(cfm_text + valToks[0].position, valToks[0].len);
                    return emitCall(builder, fCreateString,
                        {builder.CreateGlobalString(llvm::StringRef(raw.constData(), raw.length()), "", 0, module, true)});
                }
                auto ast = parseTokensToAST(valToks, cfm_text);
                return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                      application, session, url, form, variables, cfm_text);
            };

            llvm::Value *attrsVal = emitCall(builder, fCreateStruct, {});
            for (size_t ai = 0; ai < attrParts->size(); ) {
                const auto &at = (*attrParts)[ai];
                if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
                string aname(cfm_text + at.position, at.len);
                string anameLow = aname;
                anameLow.toLower();
                if (validAttrs.find(anameLow.constData()) == validAttrs.end()) {
                    string detail = "The tag does not have an attribute called ";
                    detail += aname;
                    detail += ".";
                    throw webstrada::exception("Attribute validation error for the cfquery tag.", detail);
                }
                std::vector<TextParserTokenItem> valToks;
                size_t vi = ai + 1;
                while (vi < attrParts->size()) {
                    const auto &vt = (*attrParts)[vi];
                    bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                                       vi + 1 < attrParts->size() &&
                                       isOperatorToken((*attrParts)[vi + 1].token_id));
                    if (nextIsAttr) break;
                    if (!isOperatorToken(vt.token_id)) valToks.push_back(vt);
                    vi++;
                }
                if (!valToks.empty()) {
                    llvm::Value *val = compileValue(valToks);
                    if (val) {
                        auto *keyStr = builder.CreateGlobalString(llvm::StringRef(anameLow.constData(), anameLow.length()), "", 0, module, true);
                        emitCall(builder, fIndexAssign, {attrsVal,
                            emitCall(builder, fCreateString, {keyStr}), val});
                    }
                }
                ai = vi;
            }

            // Capture the body (leading raw text is not covered by the
            // QueryExpression token, exactly like cfloop's LoopExpression), then
            // hand the buffer to cf_query_end. The body is captured VERBATIM
            // (whitespace management disabled): CF does not collapse whitespace
            // inside a <cfquery> body — it strips only the leading/trailing
            // whitespace, which cf_query_end does when it reads the buffer
            // (verified against CF 2025: internal spaces/newlines are preserved,
            // e.g. `SELECT   3` keeps its three spaces, `SELECT  #x#  AS four`
            // becomes `SELECT  4  AS four`).
            llvm::Value *capture = emitCall(builder, fQueryBegin, {});
            size_t bodyStart = qpStart.position + qpStart.len;
            size_t bodyEnd = qpEnd.position;

            WhitespaceState wsBody(false, ws.flag);
            wsBody.markTag(false, false); // left neighbour is <cfquery>
            if (hasExpr) {
                size_t bodyPos = bodyStart;
                size_t bidx = 0;
                compile_token_list(qpExpression.children, bidx, bodyPos, context, module, builder, mainfunc,
                                   capture, wsBody, cgi, server, cookie, application, session, url, form, variables,
                                   cfm_text, bodyEnd, loopStack);
                if (bodyPos < bodyEnd) {
                    wsBody.feed(module, builder, capture, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
                }
            } else {
                wsBody.feed(module, builder, capture, cfm_text + bodyStart, bodyEnd - bodyStart, WsRight::Tag);
            }
            wsBody.finish(module, builder, capture, WsRight::Tag);

            emitCall(builder, fQueryEnd, {capture, attrsVal, cgi, server, cookie, application,
                                          session, url, form, variables});
            break;
        }

        case TextParser_cfml_SharpChar: {
            // An escaped hash (##) inside an output/loop body renders a single #.
            llvm_WriteOutput(module, builder, out, "#", 1);
            break;
        }

        case TextParser_cfml_SharpExpression: {
            auto varName = extractVarFromSharpExpr(token, cfm_text);
            if (!varName.isEmpty()) {
                auto keyGlobal = builder.CreateGlobalString(llvm::StringRef(varName.constData(), varName.length()), "", 0, module, true);
                llvm_CfOutputExpr(module, builder, out, cgi, server, cookie, application, session, url, form, variables, keyGlobal);
            }
            break;
        }

        default:
            // For general sub-tokens (like EndTag) that we process as siblings, just do nothing.
            break;
        }

        // Track the last non-comment construct for whitespace management. Tags
        // that emit output make a following whitespace-only region collapse to a
        // single space: <cfoutput>/HTML <cfdump>/#expr# arm the flag (consumed by
        // the next collapsed region), <cfscript> sets a sticky flag. A <cfdump>
        // with format="text" writes through a buffered path and does neither
        // (verified against CF 2021).
        if (token.token_id != TextParser_cfml_Comment) {
            string wsTagName = string(cfm_text + token.position, token.len);
            wsTagName.toLower();
            bool arm = (token.token_id == TextParser_cfml_OutputTagPair ||
                        token.token_id == TextParser_cfml_SharpExpression ||
                        (token.token_id == TextParser_cfml_StartTag &&
                         wsTagName.startWith("<cfdump") &&
                         !isTextFormatCfdump(string(cfm_text + token.position, token.len))));
            bool sticky = (token.token_id == TextParser_cfml_ScriptTagPair);
            ws.markTag(arm, sticky);
        }

        index++;
    }
}

} // namespace webstrada
