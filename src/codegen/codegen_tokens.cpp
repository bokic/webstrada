/**
 * @file codegen_tokens.cpp
 * Code generation: tokens.
 */

#include "codegen_internal.h"

#include <webstrada/exceptions.h>
#include <webstrada/parser.h>
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


namespace webstrada {

// ---- forward declarations ----

std::vector<std::pair<std::string, std::vector<TextParserTokenItem>>>
parseTagAttrs(const std::vector<TextParserTokenItem> *attrParts, const char *cfm_text);

void parseTagAttrs(const TextParserTokenItem &startTag, const char *cfm_text,
                          std::map<std::string, std::string> &out);

// ---- definitions ----

llvm::Value *currentOut(llvm::Function *function)
{
    return g_currentOut ? g_currentOut : function->getArg(0);
}

thread_local bool g_insideCfoutput = false;

static bool isTempVariantFunction(llvm::Function *f, llvm::IRBuilder<> &builder) {
    if (!f || f->getReturnType() != builder.getPtrTy()) return false;
    llvm::StringRef name = f->getName();
    if (        name == "cfset" || name == "cfgetvar" || name == "cfoutputexpr" || name == "cfdump" ||
        name == "cf_ajaxlink" || name == "cf_ajaxonload" ||
        name.starts_with("cf_silent_") || name.starts_with("cf_savecontent_") ||
        name.starts_with("cf_xml_") || name.starts_with("cf_query_") || name == "cf_queryexecute" ||
        name.starts_with("cf_storedproc_") ||
        name.starts_with("cf_http_") ||
        name.starts_with("cf_transaction_") || name.starts_with("cf_response_") ||
        name.starts_with("cf_udf_") || name.starts_with("cf_eh_") ||
        name.starts_with("cf_component_") || name == "cf_cfinvoke" || name == "cf_cfinvoke_end" ||
        name.starts_with("cfloop_") || name.starts_with("cfforIn") ||
        name.starts_with("cfvariant_call_function") ||
        name.starts_with("cfvariant_index") ||
        name.starts_with("cfvariant_get_var") || name.starts_with("cfvariant_bare_identifier") ||
        name.starts_with("cfvariant_assign") || name.starts_with("cfvariant_copy_value") ||
        name.starts_with("cfvariant_member_method") ||
        name.starts_with("cfvariant_cleanup_") || name.starts_with("cfvariant_create_udf") ||
        name.starts_with("cfvariant_create_struct") ||
        name.starts_with("cf_custom_tag_") ||
        name == "cf_application_enable") {
        return false;
    }
    return name.starts_with("cf_") || name.starts_with("cfvariant_create_");
}

llvm::Value *emitCall(llvm::IRBuilder<> &builder, llvm::Function *f,
                      llvm::ArrayRef<llvm::Value*> args)
{
    llvm::Value *ret = nullptr;
    auto *eh = g_ehContext;
    if (eh && eh->landingPadBB) {
        auto *contBB = llvm::BasicBlock::Create(builder.getContext(), "try.cont",
                                                builder.GetInsertBlock()->getParent());
        ret = builder.CreateInvoke(f, contBB, eh->landingPadBB, args);
        builder.SetInsertPoint(contBB);
    } else if (g_currentFuncCleanupBB) {
        auto *contBB = llvm::BasicBlock::Create(builder.getContext(), "call.cont",
                                                builder.GetInsertBlock()->getParent());
        ret = builder.CreateInvoke(f, contBB, g_currentFuncCleanupBB, args);
        builder.SetInsertPoint(contBB);
    } else {
        ret = builder.CreateCall(f, args);
    }

    if (isTempVariantFunction(f, builder)) {
        auto *module = builder.GetInsertBlock()->getModule();
        auto *fRegisterTemp = getOrCreateHelper(module, builder, "cf_register_temp",
                                             builder.getVoidTy(), {builder.getPtrTy()});
        builder.CreateCall(fRegisterTemp, {ret});
    }
    return ret;
}

static llvm::Function *get_cfwriteoutput_func(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto func = module->getFunction("cfwriteoutput");
    if (func) {
        return func;
    }

    std::vector<llvm::Type*> params = {builder.getPtrTy(), builder.getPtrTy(), builder.getInt64Ty()};
    auto *ft = llvm::FunctionType::get(builder.getVoidTy(), params, false);
    func = llvm::Function::Create(ft, llvm::Function::InternalLinkage, "cfwriteoutput", module);
    return func;
}

void llvm_WriteOutput(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out, const char *str, size_t n)
{
    // Plain text written OUTSIDE a <cfoutput> body goes through the
    // cfoutputonly-gated writer so <cfsetting enablecfoutputonly> suppresses it
    // (CF's CFOutput.enablecfoutputonly). Inside <cfoutput> the write is direct.
    const char *fn = g_insideCfoutput ? "cfwriteoutput" : "cf_write_output_gated";
    auto writeoutput = getOrCreateHelper(module, builder, fn,
        builder.getVoidTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getInt64Ty()});

    std::vector<llvm::Value*> args{};
    args.push_back(out);
    args.push_back(builder.CreateGlobalString(llvm::StringRef(str, n), "", 0, module, false));
    args.push_back(llvm::ConstantInt::get(builder.getInt64Ty(), n));
    emitCall(builder, writeoutput, args);
}

WhitespaceState::WhitespaceState(bool en, WsFlag &f) : enabled(en), flag(f) {}

bool WhitespaceState::hasNewline() const {
    return ws.find('\n') != std::string::npos || ws.find('\r') != std::string::npos;
}

void WhitespaceState::emitRuntimeSpace(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out) {
    const char *fn = g_insideCfoutput ? "cf_whitespace_space" : "cf_whitespace_space_gated";
    auto f = module->getFunction(fn);
    if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, fn, module);
    emitCall(builder, f, {out});
}

void WhitespaceState::resolve(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out, WsRight right) {
    if (ws.empty()) return;
    if (left == WsContent::PlainText || right == WsRight::PlainText) {
        llvm_WriteOutput(module, builder, out, ws.c_str(), ws.size());
        return;
    }
    bool mode = flag.arm || flag.sticky;
    flag.arm = false;
    if (hasNewline()) {
        if (mode) {
            emitRuntimeSpace(module, builder, out);
        }
    } else if (left == WsContent::Tag && right == WsRight::Tag) {
        emitRuntimeSpace(module, builder, out);
    } else if (left == WsContent::Tag && right == WsRight::DocumentEnd) {
        if (mode) {
            emitRuntimeSpace(module, builder, out);
        }
    }
}

void WhitespaceState::feed(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out,
                           const char *text, size_t len, WsRight right) {
    if (len == 0) {
        if (active && right != WsRight::Comment) {
            resolve(module, builder, out, right);
            active = false;
            ws.clear();
        }
        return;
    }

    bool allWs = true;
    for (size_t i = 0; i < len; i++) {
        char c = text[i];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\f' && c != '\v') {
            allWs = false;
            break;
        }
    }

    if (!enabled) {
        if (active) {
            resolve(module, builder, out, WsRight::PlainText);
            active = false;
            ws.clear();
        }
        llvm_WriteOutput(module, builder, out, text, len);
        last = WsContent::PlainText;
        return;
    }

    if (allWs) {
        if (!active) {
            active = true;
            left = last;
        }
        ws.append(text, len);
        if (right == WsRight::Comment) return;
        resolve(module, builder, out, right);
        active = false;
        ws.clear();
    } else {
        if (active) {
            resolve(module, builder, out, WsRight::PlainText);
            active = false;
            ws.clear();
        }
        llvm_WriteOutput(module, builder, out, text, len);
        last = WsContent::PlainText;
    }
}

void WhitespaceState::finish(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out, WsRight right) {
    if (active) {
        resolve(module, builder, out, right);
        active = false;
        ws.clear();
    }
}

void WhitespaceState::markTag(bool arm, bool sticky) {
    last = WsContent::Tag;
    if (arm) flag.arm = true;
    if (sticky) flag.sticky = true;
}

static llvm::Function *get_cfset_func(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto func = module->getFunction("cfset");
    if (func) {
        return func;
    }

    std::vector<llvm::Type*> params = {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()};
    auto *ft = llvm::FunctionType::get(builder.getVoidTy(), params, false);
    func = llvm::Function::Create(ft, llvm::Function::InternalLinkage, "cfset", module);
    return func;
}

static void llvm_CfSet(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *scope, llvm::Value *key, llvm::Value *value)
{
    auto cfset = get_cfset_func(module, builder);

    std::vector<llvm::Value*> args{};
    args.push_back(scope);
    args.push_back(key);
    args.push_back(value);
    emitCall(builder, cfset, args);
}

static llvm::Function *get_cfoutputexpr_func(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto func = module->getFunction("cfoutputexpr");
    if (func) {
        return func;
    }

    std::vector<llvm::Type*> params(10, builder.getPtrTy());
    auto *ft = llvm::FunctionType::get(builder.getVoidTy(), params, false);
    func = llvm::Function::Create(ft, llvm::Function::InternalLinkage, "cfoutputexpr", module);
    return func;
}

void llvm_CfOutputExpr(llvm::Module *module, llvm::IRBuilder<> &builder,
                               llvm::Value *out, llvm::Value *cgi, llvm::Value *server,
                               llvm::Value *cookie, llvm::Value *application, llvm::Value *session,
                               llvm::Value *url, llvm::Value *form, llvm::Value *variables,
                               llvm::Value *varName)
{
    auto cfoutputexpr = get_cfoutputexpr_func(module, builder);
    std::vector<llvm::Value*> args = {out, cgi, server, cookie, application, session, url, form, variables, varName};
    emitCall(builder, cfoutputexpr, args);
}

static llvm::Function *get_cfgetvar_func(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto func = module->getFunction("cfgetvar");
    if (func) return func;

    std::vector<llvm::Type*> params = {builder.getPtrTy(), builder.getPtrTy()};
    auto *ft = llvm::FunctionType::get(builder.getPtrTy(), params, false);
    func = llvm::Function::Create(ft, llvm::Function::InternalLinkage, "cfgetvar", module);
    return func;
}

void llvm_CfGetVar(llvm::Module *module, llvm::IRBuilder<> &builder,
                           llvm::Value *scope, llvm::Value *key, llvm::Value *&result)
{
    auto cfgetvar = get_cfgetvar_func(module, builder);
    result = emitCall(builder, cfgetvar, {scope, key});
}

static llvm::Function *get_cfabort_func(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto func = module->getFunction("cfabort");
    if (func) return func;

    auto *ft = llvm::FunctionType::get(builder.getVoidTy(), false);
    func = llvm::Function::Create(ft, llvm::Function::InternalLinkage, "cfabort", module);
    return func;
}

void llvm_CfAbort(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto cfabort = get_cfabort_func(module, builder);
    emitCall(builder, cfabort, {});
    builder.CreateUnreachable();
}

static llvm::Function *get_response_flush_func(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto func = module->getFunction("cf_response_flush");
    if (func) return func;
    auto *ft = llvm::FunctionType::get(builder.getVoidTy(), {builder.getPtrTy()}, false);
    func = llvm::Function::Create(ft, llvm::Function::InternalLinkage, "cf_response_flush", module);
    return func;
}

void llvm_CfFlush(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out)
{
    auto flush = get_response_flush_func(module, builder);
    emitCall(builder, flush, {out});
}

static llvm::Function *get_response_apply_cfcontent_func(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto func = module->getFunction("cf_response_apply_cfcontent");
    if (func) return func;
    std::vector<llvm::Type*> params(6, builder.getPtrTy());
    auto *ft = llvm::FunctionType::get(builder.getVoidTy(), params, false);
    func = llvm::Function::Create(ft, llvm::Function::InternalLinkage, "cf_response_apply_cfcontent", module);
    return func;
}

void llvm_CfContent(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out,
                           llvm::Value *type, llvm::Value *reset, llvm::Value *file,
                           llvm::Value *variable, llvm::Value *deletefile)
{
    auto apply = get_response_apply_cfcontent_func(module, builder);
    emitCall(builder, apply, {out, type, reset, file, variable, deletefile});
}

llvm::Function *get_response_add_header_func(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto func = module->getFunction("cf_response_add_header");
    if (func) return func;
    auto *ft = llvm::FunctionType::get(builder.getVoidTy(), std::vector<llvm::Type*>(4, builder.getPtrTy()), false);
    func = llvm::Function::Create(ft, llvm::Function::InternalLinkage, "cf_response_add_header", module);
    return func;
}

llvm::Function *get_response_redirect_func(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto func = module->getFunction("cf_response_redirect");
    if (func) return func;
    auto *ft = llvm::FunctionType::get(builder.getVoidTy(), std::vector<llvm::Type*>(3, builder.getPtrTy()), false);
    func = llvm::Function::Create(ft, llvm::Function::InternalLinkage, "cf_response_redirect", module);
    return func;
}

std::vector<std::pair<std::string, std::vector<TextParserTokenItem>>>
parseTagAttrs(const std::vector<TextParserTokenItem> *attrParts, const char *cfm_text)
{
    std::vector<std::pair<std::string, std::vector<TextParserTokenItem>>> attrs;
    if (!attrParts) return attrs;
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        string aname(cfm_text + at.position, at.len);
        std::vector<TextParserTokenItem> valToks;
        size_t vi = ai + 1;
        while (vi < attrParts->size() && isOperatorToken((*attrParts)[vi].token_id)) vi++;
        // A new attribute begins at a Variable that is separated from the
        // previous value token by whitespace and does not continue an operator
        // expression (a="x" & y, a=foo.bar, ...). This also stops a valueless
        // trailing attribute (a="x" charset) from being swallowed as part of
        // the preceding value (CF passes such an attribute the string "true").
        bool lastWasOperator = vi > ai + 1; // an explicit '=' precedes the value
        while (vi < attrParts->size()) {
            const auto &vt = (*attrParts)[vi];
            if (isOperatorToken(vt.token_id)) {
                lastWasOperator = true;
                vi++;
                continue;
            }
            if (vt.token_id == TextParser_cfml_Variable &&
                !lastWasOperator &&
                vt.position > (*attrParts)[vi - 1].position + (*attrParts)[vi - 1].len) {
                break;
            }
            valToks.push_back(vt);
            lastWasOperator = false;
            vi++;
        }
        attrs.emplace_back(std::string(aname.constData(), aname.length()), std::move(valToks));
        ai = vi;
    }
    return attrs;
}

std::string lowercase(const std::string &s)
{
    std::string r = s;
    for (auto &c : r) c = (char)std::tolower((unsigned char)c);
    return r;
}

std::string uppercase(const std::string &s)
{
    std::string r = s;
    for (auto &c : r) c = (char)std::toupper((unsigned char)c);
    return r;
}

llvm::Function *get_cfevalbool_func(llvm::Module *module, llvm::IRBuilder<> &builder)
{
    auto func = module->getFunction("cfevalbool");
    if (func) return func;

    std::vector<llvm::Type*> params(9, builder.getPtrTy());
    auto *ft = llvm::FunctionType::get(builder.getInt32Ty(), params, false);
    func = llvm::Function::Create(ft, llvm::Function::InternalLinkage, "cfevalbool", module);
    return func;
}

static string extractVarNameFromToken(const TextParserTokenItem &valueToken, const char *text)
{
    string raw(text + valueToken.position, valueToken.len);
    raw = raw.trimmed();

    if ((raw.first() == '"' && raw.at(raw.length() - 1) == '"') ||
        (raw.first() == '\'' && raw.at(raw.length() - 1) == '\'')) {
        raw = raw.mid(1, raw.length() - 2).trimmed();
    }

    if (raw.length() >= 2 && raw.first() == '#' && raw.at(raw.length() - 1) == '#') {
        raw = raw.mid(1, raw.length() - 2).trimmed();
    }

    return raw;
}

string extractVarFromSharpExpr(const TextParserTokenItem &sharpExpr, const char *text)
{
    // The complete SharpExpression source is required for compound values
    // such as function calls. A child Variable token would otherwise reduce
    // `formatAge(1)` to `formatAge` before cfoutputexpr evaluates it.
    string source(text + sharpExpr.position, sharpExpr.len);
    source = source.trimmed();
    if (source.length() >= 2 && source.first() == '#' &&
        source.at(source.length() - 1) == '#') {
        source = source.mid(1, source.length() - 2).trimmed();
    }
    if (source.contains('(') || source.contains('[')) {
        return source;
    }

    for (auto &child : sharpExpr.children) {
        // The SharpExpression's Expression group is deleted when it wraps a
        // single child (deleteIfOnlyOneChild in cfml_definition.json), so a
        // bare literal (string, number, boolean) or a parenthesized expression
        // appears as a direct child. Extract its text so cfoutputexpr can
        // evaluate it (bare literals previously output nothing).
        switch (child.token_id) {
        case TextParser_cfml_Expression:
        case TextParser_cfml_Variable:
        case TextParser_cfml_DoubleString:
        case TextParser_cfml_SingleString:
        case TextParser_cfml_Number:
        case TextParser_cfml_Boolean:
        case TextParser_cfml_Parenthesis:
        case TextParser_cfml_ArrayIndex:
        case TextParser_cfml_CodeBlock:
            return string(text + child.position, child.len).trimmed();
        default:
            break;
        }
    }
    return string();
}

void validateOutputExpressionSharp(const TextParserTokenItem &outputExpression, const char *cfm_text)
{
    size_t start = outputExpression.position;
    size_t end = start + outputExpression.len;
    size_t covered = start;

    // Children arrive in source order; every byte not owned by a child token
    // is literal text, and a '#' in that literal text is a parse error.
    for (const auto &child : outputExpression.children) {
        if (child.position > covered) {
            for (size_t i = covered; i < child.position; ++i) {
                if (cfm_text[i] == '#') {
                    throw webstrada::exception("Parsing error!",
                        "A '#' in a <cfoutput> body must be part of a #...# expression or escaped as ##.");
                }
            }
        }
        if (child.position + child.len > covered) {
            covered = child.position + child.len;
        }
    }

    for (size_t i = covered; i < end; ++i) {
        if (cfm_text[i] == '#') {
            throw webstrada::exception("Parsing error!",
                "A '#' in a <cfoutput> body must be part of a #...# expression or escaped as ##.");
        }
    }
}

std::vector<TextParserTokenItem> mergeObjectMembers(const std::vector<TextParserTokenItem> &children)
{
    std::vector<TextParserTokenItem> processedChildren;
    for (size_t cIdx = 0; cIdx < children.size(); cIdx++) {
        const auto &tok = children[cIdx];
        if (!processedChildren.empty() && 
            (tok.token_id == TextParser_cfml_Variable || tok.token_id == TextParser_cfml_ObjectMember) &&
            (processedChildren.back().token_id == TextParser_cfml_Variable)) {
            processedChildren.back().len = tok.position + tok.len - processedChildren.back().position;
            processedChildren.back().token_id = TextParser_cfml_Variable;
        } else {
            processedChildren.push_back(tok);
        }
    }
    return processedChildren;
}

TextParserTokenItem convertToken(const textparser_token_item *src)
{
    TextParserTokenItem ret;

    ret.token_id = src->token_id;
    ret.position = textparser_get_token_position(src);
    ret.len = src->len;
    if (src->error)
        ret.error = src->error;

    auto child = src->child;
    while(child)
    {
        if (child->token_id >= TextParser_cfml_ScriptTagPair &&
            child->token_id <= TextParser_cfml_ArrayIndex) {
            if (child->token_id == TextParser_cfml_Operator && child->child) {
                auto opChild = child->child;
                while (opChild) {
                    if (opChild->token_id >= TextParser_cfml_ScriptTagPair &&
                        opChild->token_id <= TextParser_cfml_ArrayIndex) {
                        ret.children.push_back(convertToken(opChild));
                    }
                    opChild = opChild->next;
                }
            } else {
                ret.children.push_back(convertToken(child));
            }
        }
        child = child->next;
    }

    return ret;
}

static TextParserTokenItemKeyValue convertTokenKeyValue(const char *text, const textparser_token_item *src)
{
    TextParserTokenItemKeyValue ret;

    ret.token_id = src->token_id;
    ret.position = textparser_get_token_position(src);
    ret.len = src->len;
    if (src->error)
        ret.error = src->error;

    auto child = src->child;
    while(child)
    {
        if (child->token_id == TextParser_cfml_Expression)
        {
            auto key = child->child;
            auto assigment = key->next;
            auto value = assigment->next;

            std::string keyStr = std::string(text + textparser_get_token_position(key), key->len);
            std::string assigmentStr = std::string(text + textparser_get_token_position(assigment), assigment->len);

            if (assigmentStr == "=")
                ret.params.insert({keyStr, convertToken(value)});
        }

        child = child->next;
    }

    return ret;
}

std::map<string, string> parse_attributes(const string &tagText)
{
    std::map<string, string> attrs;
    size_t ai = 0;
    while (ai < tagText.length()) {
        while (ai < tagText.length() && !isalnum(tagText.at(ai))) ai++;
        if (ai >= tagText.length()) break;
        size_t ks = ai;
        while (ai < tagText.length() && isalnum(tagText.at(ai))) ai++;
        string kn = tagText.mid(ks, ai - ks);
        kn.toLower();
        while (ai < tagText.length() && (tagText.at(ai) == '=' || tagText.at(ai) == ' ' || tagText.at(ai) == '\t')) ai++;
        if (ai >= tagText.length() || (tagText.at(ai) != '"' && tagText.at(ai) != '\'')) continue;
        int q = tagText.at(ai); ai++;
        size_t vs = ai;
        while (ai < tagText.length()) {
            char c = tagText.at(ai);
            if (c == q) break;
            // A '#' opens a #...# interpolation group that is opaque to the
            // attribute value's quote character: `expression="#getSetting("confirmationMethod")#"`
            // (nested quotes inside #...# do not terminate the value — the
            // textparser grammar tokenizes the whole thing as one DoubleString,
            // and this scanner must mirror it). `##` is an escaped literal hash.
            if (c == '#') {
                ai++;
                if (ai < tagText.length() && tagText.at(ai) == '#') { ai++; continue; }
                while (ai < tagText.length() && tagText.at(ai) != '#') ai++;
                if (ai < tagText.length()) ai++; // consume the closing '#'
                continue;
            }
            ai++;
        }
        attrs[kn] = tagText.mid(vs, ai - vs);
        if (ai < tagText.length()) ai++;
    }
    return attrs;
}

bool isTextFormatCfdump(const string &tagText)
{
    auto attrs = parse_attributes(tagText);
    auto it = attrs.find("format");
    if (it == attrs.end()) return false;
    string fmt = it->second.trimmed();
    fmt.toLower();
    return fmt.equals("text");
}

std::string tokenText(const TextParserTokenItem &t, const char *cfm_text)
{
    return std::string(cfm_text + t.position, t.len);
}

bool kwTextIs(const TextParserTokenItem &t, const char *cfm_text, const char *kw)
{
    std::string s = tokenText(t, cfm_text);
    while (!s.empty() && isspace(s.front())) s.erase(s.begin());
    while (!s.empty() && isspace(s.back())) s.pop_back();
    for (auto &c : s) c = tolower((unsigned char)c);
    return s == kw;
}

std::string tagNameOf(const TextParserTokenItem &t, const char *cfm_text)
{
    std::string s(cfm_text + t.position, t.len);
    size_t start = 0;
    if (s.rfind("</", 0) == 0) start = 2;
    else if (s.rfind("<", 0) == 0) start = 1;
    size_t end = start;
    while (end < s.length() && (isalnum((unsigned char)s[end]) || s[end] == '_')) end++;
    std::string name = s.substr(start, end - start);
    for (auto &c : name) c = (char)tolower((unsigned char)c);
    return name;
}

void parseTagAttrs(const TextParserTokenItem &startTag, const char *cfm_text,
                          std::map<std::string, std::string> &out)
{
    for (const auto &ch : startTag.children) {
        if (ch.token_id != TextParser_cfml_Expression) continue;
        const auto &children = ch.children;
        for (size_t ai = 0; ai < children.size(); ) {
            if (children[ai].token_id != TextParser_cfml_Variable) { ai++; continue; }
            std::string aname(cfm_text + children[ai].position, children[ai].len);
            for (auto &c : aname) c = (char)tolower((unsigned char)c);
            ai++;
            while (ai < children.size() && isOperatorToken(children[ai].token_id)) ai++;
            if (ai >= children.size()) break;
            const auto &vt = children[ai];
            if (vt.token_id == TextParser_cfml_DoubleString || vt.token_id == TextParser_cfml_SingleString) {
                std::string v = tokenText(vt, cfm_text);
                if (v.size() >= 2) v = v.substr(1, v.size() - 2);
                out[aname] = v;
            } else {
                out[aname] = tokenText(vt, cfm_text);
            }
            ai++;
        }
    }
}

llvm::Function *getOrCreateHelper(llvm::Module *module, llvm::IRBuilder<> &builder,
                                         const char *name, llvm::Type *retTy,
                                         std::vector<llvm::Type*> params)
{
    auto *f = module->getFunction(name);
    if (!f) f = llvm::Function::Create(llvm::FunctionType::get(retTy, params, false),
                                       llvm::Function::InternalLinkage, name, module);
    return f;
}

} // namespace webstrada
