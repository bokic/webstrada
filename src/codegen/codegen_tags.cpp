/**
 * @file codegen_tags.cpp
 * Code generation: tags.
 */

#include "codegen_internal.h"

#include <webstrada/exceptions.h>
#include <webstrada/parser.h>
#include <webstrada/config.h>
#include <webstrada/template_reader.h>
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

// ---- definitions ----

size_t compile_tag_try_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    struct TagCatch {
        webstrada::string type;
        webstrada::string varName;
        std::vector<TextParserTokenItem> body;
        size_t bodyStart = 0;
        size_t bodyEnd = 0;
    };

    // ---- Scan the structure ----
    std::vector<TextParserTokenItem> tryBody;
    size_t tryBodyStart = tokens[start].position + tokens[start].len;
    size_t tryBodyEnd = 0;
    bool tryDone = false;

    std::vector<TagCatch> tagCatches;
    std::vector<TextParserTokenItem> finallyBody;
    size_t finallyStart = 0, finallyEnd = 0;
    bool hasFinally = false;

    // phase: 0 = try body (until the first <cfcatch>/<cffinally>/</cftry>),
    //        1 = inside a <cfcatch> body, 2 = inside <cffinally>
    int phase = 0;
    int depth = 0;
    size_t i = start + 1;
    while (i < tokens.size()) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag) {
            string tn(cfm_text + tok.position, tok.len);
            string tnLow = tn;
            tnLow.toLower();
            if (tnLow.startWith("<cftry")) {
                depth++;
            } else if (tnLow.startWith("<cfcatch") && depth == 0 && phase == 0) {
                if (!tryDone) {
                    tryBodyEnd = tok.position;
                    tryDone = true;
                }
                TagCatch tc;
                auto attrs = parse_attributes(tn);
                auto typeIt = attrs.find("type");
                if (typeIt != attrs.end() && !typeIt->second.trimmed().isEmpty()) {
                    tc.type = typeIt->second.trimmed();
                } else {
                    tc.type = "any";
                }
                auto varIt = attrs.find("var");
                if (varIt != attrs.end() && !varIt->second.trimmed().isEmpty()) {
                    tc.varName = varIt->second.trimmed();
                } else {
                    tc.varName = "CFCATCH";
                }
                tc.bodyStart = tok.position + tok.len;
                tagCatches.push_back(std::move(tc));
                phase = 1;
                i++;
                continue;
            } else if (tnLow.startWith("<cffinally") && depth == 0 && phase == 0) {
                if (!tryDone) {
                    tryBodyEnd = tok.position;
                    tryDone = true;
                }
                hasFinally = true;
                finallyStart = tok.position + tok.len;
                phase = 2;
                i++;
                continue;
            }
        } else if (tok.token_id == TextParser_cfml_EndTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("</cftry")) {
                if (depth > 0) {
                    depth--;
                } else {
                    if (!tryDone) tryBodyEnd = tok.position;
                    else if (phase == 1) tagCatches.back().bodyEnd = tok.position;
                    else if (phase == 2) finallyEnd = tok.position;
                    i++;
                    break;
                }
            } else if (tn.startWith("</cfcatch") && depth == 0 && phase == 1) {
                tagCatches.back().bodyEnd = tok.position;
                phase = 0;
                i++;
                continue;
            } else if (tn.startWith("</cffinally") && depth == 0 && phase == 2) {
                finallyEnd = tok.position;
                phase = 0;
                i++;
                continue;
            }
        }
        if (phase == 1) tagCatches.back().body.push_back(tok);
        else if (phase == 2) finallyBody.push_back(tok);
        else if (!tryDone) tryBody.push_back(tok);
        i++;
    }
    const size_t nextIdx = i;

    // CF rejects a <cftry> that contains neither a <cfcatch> nor a <cffinally>
    // at compile time (verified on the RDS host, CF 2025 — the page 500s):
    // "Context validation error in CFTRY block. A CFTRY must contain at least
    // one CFCATCH clause or CFFINALLY clause."
    if (tagCatches.empty() && !hasFinally) {
        throw webstrada::exception("Context validation error in CFTRY block. A CFTRY must contain at least one CFCATCH clause or CFFINALLY clause.");
    }

    // Compiles a body region (a token slice plus the raw source range it spans).
    auto compileTagBody = [&](const std::vector<TextParserTokenItem> &bodyToks,
                              size_t bodyStart, size_t bodyEnd) {
        WhitespaceState wsBody(ws.enabled, ws.flag);
        wsBody.markTag(false, false); // left neighbour is a CFML tag
        size_t bodyPos = bodyStart;
        size_t bidx = 0;
        compile_token_list(bodyToks, bidx, bodyPos, context, module, builder, mainfunc,
                           out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                           cfm_text, bodyEnd, loopStack);
        if (bodyPos < bodyEnd) {
            wsBody.feed(module, builder, out, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
        }
        wsBody.finish(module, builder, out, WsRight::Tag);
    };

    // ---- Codegen (shared with the script form) ----
    std::vector<TryCatchClause> clauses;
    clauses.reserve(tagCatches.size());
    for (const auto &tc : tagCatches) {
        clauses.push_back({tc.type, tc.varName});
    }

    emit_try_catch_codegen(context, module, builder, mainfunc, out,
                           cgi, server, cookie, application, session, url, form, variables,
                           cfm_text, cfm_text_size, loopStack, clauses, hasFinally,
                           [&]() { compileTagBody(tryBody, tryBodyStart, tryBodyEnd); },
                           [&](size_t c, llvm::Value *) {
                               compileTagBody(tagCatches[c].body, tagCatches[c].bodyStart, tagCatches[c].bodyEnd);
                           },
                           [&]() { compileTagBody(finallyBody, finallyStart, finallyEnd); });
    return nextIdx;
}

size_t compile_tag_silent_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);

    // Attribute validation: <cfsilent> has no attributes; CF rejects any that
    // appear (empty response / compile error). Self-closing `<cfsilent/>` and a
    // trailing space before `>` are fine.
    {
        size_t p = 9; // strlen("<cfsilent")
        while (p < startText.length() &&
               (startText.at(p) == ' ' || startText.at(p) == '\t' ||
                startText.at(p) == '\r' || startText.at(p) == '\n')) p++;
        bool selfClose = (p + 1 < startText.length()) &&
                         startText.at(p) == '/' && startText.at(p + 1) == '>';
        if (p < startText.length() && startText.at(p) != '>' && !selfClose) {
            size_t q = p;
            while (q < startText.length() &&
                   (isalnum((unsigned char)startText.at(q)) || startText.at(q) == '_')) q++;
            string attrName = startText.mid(p, q - p);
            if (attrName.isEmpty()) attrName = "?";
            string detail = "The tag does not have an attribute called ";
            detail += attrName;
            detail += ". The valid attribute(s) are .";
            throw webstrada::exception("Attribute validation error for the cfsilent tag.", detail);
        }
    }

    // Scan for the matching `</cfsilent>`, tracking nested `<cfsilent>`
    // (self-closing ones push nothing). An unterminated <cfsilent> takes the
    // rest of the template as its body, like CF.
    std::vector<TextParserTokenItem> body;
    size_t bodyStart = tokens[start].position + tokens[start].len;
    size_t bodyEnd = 0;
    int depth = 0;
    size_t nextIdx = start + 1;
    bool foundEnd = false;
    if (startText.endsWith("/>")) {
        // Self-closing `<cfsilent/>`: empty body, nothing to suppress.
        nextIdx = start + 1;
        foundEnd = true;
        bodyEnd = bodyStart;
    }
    for (size_t i = start + 1; !foundEnd && i < tokens.size(); i++) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("<cfsilent") && !tn.endsWith("/>")) {
                depth++;
            }
        } else if (tok.token_id == TextParser_cfml_EndTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("</cfsilent")) {
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
    if (!foundEnd) {
        bodyEnd = cfm_text_size;
        nextIdx = tokens.size();
    }

    // Redirect the body's output to a runtime discard buffer, compile the body
    // with its own WhitespaceState, then pop the buffer.
    if (bodyEnd > bodyStart || !body.empty()) {
        auto *fBegin = module->getFunction("cf_silent_begin");
        if (!fBegin) fBegin = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                                                     llvm::Function::InternalLinkage, "cf_silent_begin", module);
        auto *fEnd = module->getFunction("cf_silent_end");
        if (!fEnd) fEnd = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {}, false),
                                                 llvm::Function::InternalLinkage, "cf_silent_end", module);

        llvm::Value *discard = emitCall(builder, fBegin, {out});
        WhitespaceState wsBody(ws.enabled, ws.flag);
        wsBody.markTag(false, false); // left neighbour is <cfsilent>
        size_t bodyPos = bodyStart;
        size_t bidx = 0;
        compile_token_list(body, bidx, bodyPos, context, module, builder, mainfunc,
                           discard, wsBody, cgi, server, cookie, application, session, url, form, variables,
                           cfm_text, bodyEnd, loopStack);
        if (bodyPos < bodyEnd) {
            wsBody.feed(module, builder, discard, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
        }
        wsBody.finish(module, builder, discard, WsRight::Tag);
        emitCall(builder, fEnd, {});
    }
    return nextIdx;
}

size_t compile_tag_savecontent_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);

    // Attribute validation: `variable` is required; unknown attributes are
    // compile-time errors matching CF 2025.
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    auto tagAttrs = parseTagAttrs(attrParts, cfm_text);
    const std::vector<TextParserTokenItem> *varToks = nullptr;
    bool foundVar = false;
    for (const auto &a : tagAttrs) {
        if (lowercase(a.first) == "variable") {
            varToks = &a.second;
            foundVar = true;
        } else {
            throw webstrada::exception(("Attribute validation error for tag CFSAVECONTENT. It does not allow the attribute(s) " + uppercase(a.first) + ". The valid attribute(s) are VARIABLE.").c_str());
        }
    }
    if (!foundVar) {
        throw webstrada::exception("Attribute validation error for tag CFSAVECONTENT. It requires the attribute(s): VARIABLE.");
    }

    // The variable name value is evaluated up front (CF validates it at tag
    // start), so it is available on both the success and the exception path.
    llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
    llvm::Value *varVal = nullPtr;
    if (varToks && !varToks->empty()) {
        if (varToks->size() == 1 &&
            ((*varToks)[0].token_id == TextParser_cfml_DoubleString ||
             (*varToks)[0].token_id == TextParser_cfml_SingleString)) {
            auto node = std::make_unique<ExprAST>();
            node->type = ExprAST::LiteralString;
            node->token = (*varToks)[0];
            varVal = CompileExprAST(module, builder, mainfunc, node, cgi, server, cookie,
                                    application, session, url, form, variables, cfm_text);
        } else {
            auto ast = parseTokensToAST(*varToks, cfm_text);
            varVal = CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                                    application, session, url, form, variables, cfm_text);
        }
    }

    // Scan for the matching `</cfsavecontent>`, tracking nested ones. An
    // unterminated <cfsavecontent> takes the rest of the template, like CF.
    std::vector<TextParserTokenItem> body;
    size_t bodyStart = tokens[start].position + tokens[start].len;
    size_t bodyEnd = 0;
    int depth = 0;
    size_t nextIdx = start + 1;
    bool foundEnd = false;
    if (startText.endsWith("/>")) {
        foundEnd = true;
        bodyEnd = bodyStart;
    }
    for (size_t i = start + 1; !foundEnd && i < tokens.size(); i++) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("<cfsavecontent") && !tn.endsWith("/>")) depth++;
        } else if (tok.token_id == TextParser_cfml_EndTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("</cfsavecontent")) {
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
    if (!foundEnd) {
        bodyEnd = cfm_text_size;
        nextIdx = tokens.size();
    }

    auto *fValidate = getOrCreateHelper(module, builder, "cf_savecontent_validate", builder.getVoidTy(),
        {builder.getPtrTy()});
    auto *fBegin = getOrCreateHelper(module, builder, "cf_savecontent_begin", builder.getPtrTy(),
        {builder.getPtrTy()});
    auto *fEnd = getOrCreateHelper(module, builder, "cf_savecontent_end", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy()});
    auto *fThrow = getOrCreateHelper(module, builder, "cf_eh_throw", builder.getVoidTy(),
        {builder.getPtrTy()});

    emitCall(builder, fValidate, {varVal});
    llvm::Value *capture = emitCall(builder, fBegin, {out});

    std::vector<llvm::Value*> assignArgs = {capture, cgi, server, cookie, application,
                                             session, url, form, variables, varVal};

    // The body compiles into the capture buffer with its own whitespace state
    // (left neighbour is the <cfsavecontent> start tag), then the capture is
    // stored into the `variable` name. The body is wrapped in a catch-all so an
    // exception still stores the partial content (CF's SaveContentTag.doCatch)
    // before the exception propagates.
    auto compileBody = [&]() {
        WhitespaceState wsBody(ws.enabled, ws.flag);
        wsBody.markTag(false, false);
        size_t bodyPos = bodyStart;
        size_t bidx = 0;
        compile_token_list(body, bidx, bodyPos, context, module, builder, mainfunc,
                           capture, wsBody, cgi, server, cookie, application, session, url, form, variables,
                           cfm_text, bodyEnd, loopStack);
        if (bodyPos < bodyEnd) {
            wsBody.feed(module, builder, capture, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
        }
        wsBody.finish(module, builder, capture, WsRight::Tag);
    };

    emit_try_catch_codegen(context, module, builder, mainfunc, out,
                           cgi, server, cookie, application, session, url, form, variables,
                            cfm_text, cfm_text_size, loopStack,
                            {{"any", ""}}, false,
                            compileBody,
                            [&](size_t, llvm::Value *captured) {
                                // Store the partial content AND pop the capture
                                // buffer (cf_savecontent_end does both), then
                                // re-raise the exception (CF's SaveContentTag.
                                // doCatch). The success path pops it below.
                                emitCall(builder, fEnd, assignArgs);
                                emitCall(builder, fThrow, {captured});
                                builder.CreateUnreachable();
                            },
                            []() {});
    emitCall(builder, fEnd, assignArgs);
    return nextIdx;
}

struct CustomTagInvokeCfg {
    llvm::Value *tagPathVar = nullptr;   // runtime cfvariant path (overrides tagPathStr)
    std::string tagPathStr;              // static tag template path
    llvm::Value *tagNameVar = nullptr;   // runtime cfvariant name basis (overrides tagNameStr)
    std::string tagNameStr;              // static public-name basis (uppercased)
    llvm::Value *attrsVal = nullptr;     // attributes struct
    bool isModule = false;               // <cfmodule> naming (CF_<NAME> vs cf_<name>)
    const char *templateNameHint = nullptr; // static filename hint for the public name
    bool isSelfClosing = false;
};

// True when the lowercased tag text `tn` starts with `pattern` AND the
// character immediately after the pattern is a tag-name delimiter (end of
// text, `>`, `/>`, or whitespace). Guards the paired-tag scan against prefix
// collisions: `<mango:page>` and `<mango:page ...>` match "<mango:page", but
// `<mango:pageproperty>` does not (it is a different tag).
bool tagTextAtNameBoundary(const webstrada::string &tn, const char *pattern)
{
    size_t plen = strlen(pattern);
    if (tn.length() < (int)plen || !tn.startWith(pattern)) return false;
    if (tn.length() == (int)plen) return true;
    char c = tn.constData()[plen];
    return c == '>' || c == '/' || c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Emits the IR for a custom-tag invocation: start template, optional body
// capture and end template (with cfexit method="loop" support), and the final
// output emission matching CF's ModuleTag.doAfterBody order. Shared by the
// <prefix:tag> and <cfmodule> forms; `openPattern`/`closePattern` (lowercased)
// drive the paired-tag scan.
size_t compile_custom_tag_invoke_ir(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    const std::string &openPattern,
    const std::string &closePattern,
    const CustomTagInvokeCfg &cfg,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);

    auto *fInvoke = getOrCreateHelper(module, builder, "cf_custom_tag_invoke", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getInt1Ty(), builder.getInt1Ty(),
         builder.getInt1Ty(), builder.getPtrTy()});
    auto *fFinish = getOrCreateHelper(module, builder, "cf_custom_tag_finish", builder.getVoidTy(), {});
    auto *fShouldLoop = getOrCreateHelper(module, builder, "cf_custom_tag_should_loop", builder.getInt1Ty(), {});
    auto *fShouldSkip = getOrCreateHelper(module, builder, "cf_custom_tag_should_skip_body", builder.getInt1Ty(), {});
    auto *fSavecontentBegin = getOrCreateHelper(module, builder, "cf_savecontent_begin", builder.getPtrTy(), {builder.getPtrTy()});
    auto *fSavecontentEnd = getOrCreateHelper(module, builder, "cf_savecontent_end", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy()});
    auto *fThrow = getOrCreateHelper(module, builder, "cf_eh_throw", builder.getVoidTy(), {builder.getPtrTy()});
    auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());

    // The tag template path passed to the runtime. A runtime cfvariant
    // (cfmodule's template/name attribute) goes in the tagPathVar slot; a
    // static global string (a prefixed tag's taglib/tagName) in the tagPath slot.
    llvm::Value *tagPathVar = cfg.tagPathVar ? cfg.tagPathVar : nullPtr;
    llvm::Value *tagPathStrVal = nullPtr;
    if (!cfg.tagPathVar) {
        tagPathStrVal = builder.CreateGlobalString(llvm::StringRef(cfg.tagPathStr.c_str(), cfg.tagPathStr.size()), "", 0, module, true);
    }
    llvm::Value *tagNameVal = cfg.tagNameVar;
    if (!tagNameVal) {
        tagNameVal = builder.CreateGlobalString(llvm::StringRef(cfg.tagNameStr.c_str(), cfg.tagNameStr.size()), "", 0, module, true);
    }
    llvm::Value *nameHintVal = nullPtr;
    if (cfg.templateNameHint) {
        nameHintVal = builder.CreateGlobalString(llvm::StringRef(cfg.templateNameHint, strlen(cfg.templateNameHint)), "", 0, module, true);
    }

    // A self-closing tag is a pair with an empty body: ColdFusion runs the
    // start template and then the end template with hasEndTag=YES.
    bool isSelfClosing = cfg.isSelfClosing;

    std::vector<TextParserTokenItem> body;
    size_t bodyStart = tokens[start].position + tokens[start].len;
    size_t bodyEnd = 0;
    size_t nextIdx = start + 1;
    bool hasEndTag = true;
    bool foundEnd = false;

    if (!isSelfClosing) {
        // Scan for matching closing tag.
        int depth = 0;
        for (size_t i = start + 1; !foundEnd && i < tokens.size(); i++) {
            const auto &tok = tokens[i];
            if (tok.token_id == TextParser_cfml_StartTag) {
                string tn(cfm_text + tok.position, tok.len);
                tn.toLower();
                if (tagTextAtNameBoundary(tn, openPattern.c_str()) && !tn.endsWith("/>")) depth++;
            } else if (tok.token_id == TextParser_cfml_EndTag) {
                string tn(cfm_text + tok.position, tok.len);
                tn.toLower();
                if (tagTextAtNameBoundary(tn, closePattern.c_str())) {
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
        if (!foundEnd) {
            // Unterminated tag: ColdFusion treats it as a single (start-only)
            // invocation.
            hasEndTag = false;
        }
    } else {
        bodyEnd = bodyStart;
        foundEnd = true;
    }

    // 1. Invoke start mode.
    emitCall(builder, fInvoke, {out, cgi, server, cookie, application, session, url, form, variables,
                                tagPathVar, tagPathStrVal, tagNameVal, cfg.attrsVal, nullPtr, builder.getInt1(hasEndTag), builder.getInt1(false),
                                builder.getInt1(cfg.isModule), nameHintVal});

    llvm::BasicBlock *loopBodyBB = llvm::BasicBlock::Create(context, "customtag.body", mainfunc);
    llvm::BasicBlock *loopContBB = llvm::BasicBlock::Create(context, "customtag.cont", mainfunc);

    if (!hasEndTag) {
        // Single invocation only.
        builder.CreateBr(loopContBB);
    } else {
        // A bare <cfexit>/exittag in the start template skips the body and the
        // end tag entirely (ColdFusion's doStartTag returns SKIP_BODY).
        llvm::Value *skipBody = emitCall(builder, fShouldSkip, {});
        builder.CreateCondBr(skipBody, loopContBB, loopBodyBB);
    }

    // 2. Loop block for body capture & end-mode execution (supporting cfexit method="loop").
    builder.SetInsertPoint(loopBodyBB);

    // Capture body output into buffer.
    llvm::Value *capture = emitCall(builder, fSavecontentBegin, {out});

    if (isSelfClosing || (bodyEnd > bodyStart)) {
        auto compileBody = [&]() {
            WhitespaceState wsBody(ws.enabled, ws.flag);
            wsBody.markTag(false, false);
            size_t bodyPos = bodyStart;
            size_t bidx = 0;
            compile_token_list(body, bidx, bodyPos, context, module, builder, mainfunc,
                               capture, wsBody, cgi, server, cookie, application, session, url, form, variables,
                               cfm_text, bodyEnd, loopStack);
            if (bodyPos < bodyEnd) {
                wsBody.feed(module, builder, capture, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
            }
            wsBody.finish(module, builder, capture, WsRight::Tag);
        };

        emit_try_catch_codegen(context, module, builder, mainfunc, out,
                               cgi, server, cookie, application, session, url, form, variables,
                               cfm_text, cfm_text_size, loopStack,
                               {{"any", ""}}, false,
                               compileBody,
                               [&](size_t, llvm::Value *captured) {
                                   emitCall(builder, fSavecontentEnd, {capture, cgi, server, cookie, application, session, url, form, variables, nullPtr});
                                   emitCall(builder, fFinish, {});
                                   emitCall(builder, fThrow, {captured});
                                   builder.CreateUnreachable();
                               },
                               []() {});
    }

    // 3. Execute End Mode. Runs while the capture buffer is still alive (it is
    //    read to seed thisTag.generatedContent); the runtime emits the body
    //    (or the replacement generatedContent) followed by the end template's
    //    own output — CF's ModuleTag.doAfterBody order.
    emitCall(builder, fInvoke, {out, cgi, server, cookie, application, session, url, form, variables,
                                tagPathVar, tagPathStrVal, tagNameVal, cfg.attrsVal, capture, builder.getInt1(true), builder.getInt1(true),
                                builder.getInt1(cfg.isModule), nameHintVal});

    // Pop capture buffer (without assigning to a CFML variable).
    emitCall(builder, fSavecontentEnd, {capture, cgi, server, cookie, application, session, url, form, variables, nullPtr});

    llvm::BasicBlock *checkLoopBB = llvm::BasicBlock::Create(context, "customtag.checkloop", mainfunc);

    builder.CreateBr(checkLoopBB);

    builder.SetInsertPoint(checkLoopBB);
    // 4. Check should loop.
    llvm::Value *shouldLoop = emitCall(builder, fShouldLoop, {});
    builder.CreateCondBr(shouldLoop, loopBodyBB, loopContBB);

    builder.SetInsertPoint(loopContBB);
    // Finish custom tag.
    emitCall(builder, fFinish, {});

    return nextIdx;
}

size_t compile_custom_tag_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    const std::string &prefix,
    const std::string &tagName,
    const std::string &taglib,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);

    // Compute tag template path: taglib + "/" + tagName + ".cfm"
    std::string tagTemplatePath = taglib;
    if (!tagTemplatePath.empty() && tagTemplatePath.back() != '/') {
        tagTemplatePath += "/";
    }
    tagTemplatePath += tagName;
    tagTemplatePath += ".cfm";

    // The public-name basis for thisTag/GetBaseTagList: the tag name without
    // the prefix (CF names it CF_<NAME>).
    std::string fullTagName = tagName;
    for (auto &c : fullTagName) c = toupper((unsigned char)c);

    // Parse attributes
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    auto tagAttrs = parseTagAttrs(attrParts, cfm_text);

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

    auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
    auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                           {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
    auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});

    llvm::Value *attrsVal = emitCall(builder, fCreateStruct, {});
    for (const auto &a : tagAttrs) {
        // A valueless attribute (a="x" charset) is passed to the tag as the
        // string "true" (verified against CF 2025: len(attributes.charset)==4).
        llvm::Value *val = compileValue(a.second);
        if (!val) val = emitCall(builder, fCreateString, {builder.CreateGlobalString("true", "", 0, module, true)});
        std::string upperKey = a.first;
        for (auto &c : upperKey) c = toupper(c);
        llvm::Value *keyVal = emitCall(builder, fCreateString,
            {builder.CreateGlobalString(llvm::StringRef(upperKey.c_str(), upperKey.size()), "", 0, module, true)});
        emitCall(builder, fIndexAssign, {attrsVal, keyVal, val});
    }

    CustomTagInvokeCfg cfg;
    cfg.tagPathStr = tagTemplatePath;
    cfg.tagNameStr = fullTagName;
    cfg.attrsVal = attrsVal;
    cfg.isSelfClosing = startText.endsWith("/>");

    // Adobe CF also supports the legacy direct custom-tag spelling
    // <cf_name> (the template is name.cfm).  It has no colon, so its paired
    // tag pattern is different from the imported-prefix form.
    std::string openPattern;
    std::string closePattern;
    if (prefix == "cf_") {
        openPattern = "<cf_" + tagName;
        closePattern = "</cf_" + tagName;
    } else {
        openPattern = "<" + prefix + ":" + tagName;
        closePattern = "</" + prefix + ":" + tagName;
    }
    for (auto &c : openPattern) c = tolower(c);
    for (auto &c : closePattern) c = tolower(c);

    return compile_custom_tag_invoke_ir(tokens, start, openPattern, closePattern, cfg,
                                        context, module, builder, mainfunc, out, ws,
                                        cgi, server, cookie, application, session, url, form, variables,
                                        cfm_text, cfm_text_size, loopStack);
}

size_t compile_cfmodule_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);

    // Parse attributes.
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    auto tagAttrs = parseTagAttrs(attrParts, cfm_text);

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

    auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
    auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                           {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
    auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
    auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());

    // Build the attributes struct from every attribute except
    // template/name/attributecollection (which configure the module).
    llvm::Value *aTemplate = nullptr, *aName = nullptr, *aArgColl = nullptr;
    llvm::Value *attrsVal = emitCall(builder, fCreateStruct, {});
    for (const auto &a : tagAttrs) {
        std::string low = lowercase(a.first);
        if (low == "template" || low == "name" || low == "attributecollection") continue;
        // A valueless attribute is passed to the module as the string "true"
        // (verified against CF 2025: len(attributes.charset)==4).
        llvm::Value *val = compileValue(a.second);
        if (!val) val = emitCall(builder, fCreateString, {builder.CreateGlobalString("true", "", 0, module, true)});
        std::string upperKey = a.first;
        for (auto &c : upperKey) c = toupper(c);
        llvm::Value *keyVal = emitCall(builder, fCreateString,
            {builder.CreateGlobalString(llvm::StringRef(upperKey.c_str(), upperKey.size()), "", 0, module, true)});
        emitCall(builder, fIndexAssign, {attrsVal, keyVal, val});
    }
    // Collect the special attributes (may be dynamic expressions).
    for (const auto &a : tagAttrs) {
        std::string low = lowercase(a.first);
        llvm::Value *val = compileValue(a.second);
        if (low == "template") aTemplate = val;
        else if (low == "name") aName = val;
        else if (low == "attributecollection") aArgColl = val;
    }

    // Merge attributecollection into the attributes (explicit attrs win).
    if (aArgColl) {
        auto *fMerge = getOrCreateHelper(module, builder, "cf_custom_tag_merge_attributecollection", builder.getVoidTy(),
            {builder.getPtrTy(), builder.getPtrTy()});
        emitCall(builder, fMerge, {attrsVal, aArgColl});
    }

    // Compute the tag template path from template/name at runtime.
    auto *fPath = getOrCreateHelper(module, builder, "cf_custom_tag_module_path", builder.getPtrTy(),
        {builder.getPtrTy(), builder.getPtrTy()});
    llvm::Value *pathVal = emitCall(builder, fPath, {aTemplate ? aTemplate : nullPtr,
                                                     aName ? aName : nullPtr});

    CustomTagInvokeCfg cfg;
    cfg.tagPathVar = pathVal;
    cfg.attrsVal = attrsVal;
    cfg.isModule = true;
    cfg.isSelfClosing = startText.endsWith("/>");

    std::string openPattern = "<cfmodule";
    std::string closePattern = "</cfmodule";

    return compile_custom_tag_invoke_ir(tokens, start, openPattern, closePattern, cfg,
                                        context, module, builder, mainfunc, out, ws,
                                        cgi, server, cookie, application, session, url, form, variables,
                                        cfm_text, cfm_text_size, loopStack);
}

size_t compile_tag_processingdirective_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);
    auto attrs = parse_attributes(startText);

    // Unknown attributes are compile-time errors; only suppressWhitespace is in
    // CF's ProcessingDirectiveTag bean (pageEncoding is a compile-time
    // directive handled below).
    for (const auto &a : attrs) {
        if (!a.first.equals("suppresswhitespace") && !a.first.equals("pageencoding")) {
            std::string aname = std::string(a.first.constData(), a.first.length());
            throw webstrada::exception(("Attribute validation error for tag CFPROCESSINGDIRECTIVE. It does not allow the attribute(s) " + uppercase(aname) + ". The valid attribute(s) are SUPPRESSWHITESPACE.").c_str());
        }
    }

    // Whitespace management for the body: suppressWhitespace="yes"/absent keeps
    // the default management (collapse), "no" disables it (verbatim). The value
    // must be a static literal; a dynamic value cannot be compiled (whitespace
    // management is a compile-time decision in this engine).
    bool manageWs = config::enableWhitespaceManagement;
    auto swIt = attrs.find("suppresswhitespace");
    if (swIt != attrs.end()) {
        string v = swIt->second.trimmed();
        if (v.indexOf('#') >= 0) {
            throw webstrada::exception("cfprocessingdirective",
                "The SUPPRESSWHITESPACE attribute must be a literal value (dynamic whitespace management is not supported).");
        }
        string low = v;
        low.toLower();
        bool isBool = low.equals("yes") || low.equals("true") || low.equals("1") ||
                      low.equals("on") || low.equals("no") || low.equals("false") ||
                      low.equals("0") || low.equals("off");
        if (!isBool) {
            throw webstrada::exception("Attribute validation error for CFPROCESSINGDIRECTIVE. The value of the SUPPRESSWHITESPACE attribute is invalid. The value cannot be converted to a boolean because it is not a simple value.Simple values are booleans, numbers, strings, and date-time values.");
        }
        if (low.equals("no") || low.equals("false") || low.equals("0") || low.equals("off")) {
            manageWs = false;
        }
    }

    // pageEncoding is a compile-time directive naming the source charset. The
    // template_reader (llvm_codegen::compile) already decoded the page with
    // this charset and ran CF's BOM-conflict check before code generation, so
    // here it only validates the name as a backstop (the value must still be a
    // supported charset for templates compiled without the reader).
    auto peIt = attrs.find("pageencoding");
    if (peIt != attrs.end()) {
        string enc = peIt->second.trimmed();
        std::string en = std::string(enc.constData(), enc.length());
        if (en.find('#') != std::string::npos) {
            throw webstrada::exception("cfprocessingdirective",
                "The PAGEENCODING attribute must be a literal value.");
        }
        std::string err = pageEncodingError(en);
        if (!err.empty()) {
            throw webstrada::exception(err.c_str());
        }
    }

    // Scan for the matching `</cfprocessingdirective>` (self-closing is an
    // empty body).
    std::vector<TextParserTokenItem> body;
    size_t bodyStart = tokens[start].position + tokens[start].len;
    size_t bodyEnd = 0;
    int depth = 0;
    size_t nextIdx = start + 1;
    bool foundEnd = false;
    if (startText.endsWith("/>")) {
        foundEnd = true;
        bodyEnd = bodyStart;
    }
    for (size_t i = start + 1; !foundEnd && i < tokens.size(); i++) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("<cfprocessingdirective") && !tn.endsWith("/>")) depth++;
        } else if (tok.token_id == TextParser_cfml_EndTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("</cfprocessingdirective")) {
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
    if (!foundEnd) {
        bodyEnd = cfm_text_size;
        nextIdx = tokens.size();
    }

    // Compile the body with whitespace management enabled/disabled. With
    // management disabled the whitespace is written verbatim (CF's
    // suppressWhitespace="no").
    WhitespaceState wsBody(manageWs, ws.flag);
    wsBody.markTag(false, false);
    size_t bodyPos = bodyStart;
    size_t bidx = 0;
    compile_token_list(body, bidx, bodyPos, context, module, builder, mainfunc,
                       out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                       cfm_text, bodyEnd, loopStack);
    if (bodyPos < bodyEnd) {
        wsBody.feed(module, builder, out, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
    }
    wsBody.finish(module, builder, out, WsRight::Tag);
    return nextIdx;
}

size_t compile_tag_xml_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);

    // Attribute parsing: `variable` (required) and `casesensitive` (optional,
    // boolean). Attribute values are compiled like <cfthrow>/<cfinclude> values,
    // so `variable="#varName#"` evaluates and `casesensitive="yes"` stays a
    // string that cf_xml_end converts with CF's strict boolean cast. Unknown
    // attributes are compile-time errors (CF rejects them).
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
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
    llvm::Value *varVal = nullptr, *caseSensVal = nullptr;
    bool hasVariable = false;
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        string aname(cfm_text + at.position, at.len);
        string anameLow = aname;
        anameLow.toLower();
        if (!anameLow.equals("variable") && !anameLow.equals("casesensitive")) {
            string detail = "The tag does not have an attribute called ";
            detail += aname;
            detail += ". The valid attribute(s) are variable, casesensitive.";
            throw webstrada::exception("Attribute validation error for the cfxml tag.", detail);
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
        llvm::Value *val = compileValue(valToks);
        if (anameLow.equals("variable")) {
            varVal = val;
            hasVariable = true;
        } else if (anameLow.equals("casesensitive")) {
            caseSensVal = val;
        }
        ai = vi;
    }
    if (!hasVariable) {
        throw webstrada::exception("cfxml", "Missing required attribute (variable).");
    }
    auto *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());

    // Scan for the matching `</cfxml>`, tracking nested `<cfxml>` (self-closing
    // ones push nothing). An unterminated <cfxml> takes the rest of the template
    // as its body, like CF.
    std::vector<TextParserTokenItem> body;
    size_t bodyStart = tokens[start].position + tokens[start].len;
    size_t bodyEnd = 0;
    int depth = 0;
    size_t nextIdx = start + 1;
    bool foundEnd = false;
    if (startText.endsWith("/>")) {
        nextIdx = start + 1;
        foundEnd = true;
        bodyEnd = bodyStart;
    }
    for (size_t i = start + 1; !foundEnd && i < tokens.size(); i++) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("<cfxml") && !tn.endsWith("/>")) {
                depth++;
            }
        } else if (tok.token_id == TextParser_cfml_EndTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("</cfxml")) {
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
    if (!foundEnd) {
        bodyEnd = cfm_text_size;
        nextIdx = tokens.size();
    }

    // Redirect the body's output to a capture buffer, compile the body with its
    // own WhitespaceState, then hand the buffer to cf_xml_end (which pops it,
    // trims, parses the XML and assigns the document to `variable`).
    auto *fBegin = module->getFunction("cf_xml_begin");
    if (!fBegin) fBegin = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {}, false),
                                                 llvm::Function::InternalLinkage, "cf_xml_begin", module);
    llvm::Value *capture = emitCall(builder, fBegin, {});

    WhitespaceState wsBody(ws.enabled, ws.flag);
    wsBody.markTag(false, false); // left neighbour is <cfxml>
    size_t bodyPos = bodyStart;
    size_t bidx = 0;
    compile_token_list(body, bidx, bodyPos, context, module, builder, mainfunc,
                       capture, wsBody, cgi, server, cookie, application, session, url, form, variables,
                       cfm_text, bodyEnd, loopStack);
    if (bodyPos < bodyEnd) {
        wsBody.feed(module, builder, capture, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
    }
    wsBody.finish(module, builder, capture, WsRight::Tag);

    auto *fXmlEnd = getOrCreateHelper(module, builder, "cf_xml_end", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
    emitCall(builder, fXmlEnd, {capture, cgi, server, cookie, application, session,
                                url, form, variables, varVal, caseSensVal ? caseSensVal : nullPtr});
    return nextIdx;
}

size_t compile_tag_http_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);

    // The valid attribute set (CF's HttpTag bean); unknown attributes are
    // compile-time errors like CF.
    static const std::unordered_set<std::string> httpValidAttrs = {
        "url", "port", "method", "proxyserver", "proxyport", "proxyuser",
        "proxypassword", "username", "password", "useragent", "charset",
        "resolveurl", "throwonerror", "redirect", "timeout", "getasbinary",
        "result", "delimiter", "name", "columns", "firstrowasheaders",
        "textqualifier", "file", "multipart", "multiparttype",
        "clientcertpassword", "path", "clientcert", "compression", "authtype",
        "domain", "workstation", "cachedwithin", "encodeurl"
    };

    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }

    auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
    auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                           {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
    auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});

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
        if (httpValidAttrs.find(anameLow.constData()) == httpValidAttrs.end()) {
            string detail = "The tag does not have an attribute called ";
            detail += aname;
            detail += ".";
            throw webstrada::exception("Attribute validation error for the cfhttp tag.", detail);
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

    // Scan for the matching `</cfhttp>`, tracking nested `<cfhttp>` (a
    // self-closing one pushes nothing). An unterminated <cfhttp> takes the rest
    // of the template as its body.
    std::vector<TextParserTokenItem> body;
    size_t bodyStart = tokens[start].position + tokens[start].len;
    size_t bodyEnd = 0;
    int depth = 0;
    size_t nextIdx = start + 1;
    bool foundEnd = false;
    if (startText.endsWith("/>")) {
        nextIdx = start + 1;
        foundEnd = true;
        bodyEnd = bodyStart;
    }
    for (size_t i = start + 1; !foundEnd && i < tokens.size(); i++) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("<cfhttp") && !tn.startWith("<cfhttpparam") && !tn.endsWith("/>")) {
                depth++;
            }
        } else if (tok.token_id == TextParser_cfml_EndTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("</cfhttp")) {
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
    if (!foundEnd) {
        bodyEnd = cfm_text_size;
        nextIdx = tokens.size();
    }

    // Redirect the body's output to a discard buffer (the cfhttp body produces
    // no page output, like CF), bracket with cf_http_begin/end.
    auto *fBegin = getOrCreateHelper(module, builder, "cf_http_begin", builder.getVoidTy(), {builder.getPtrTy()});
    auto *fEnd = getOrCreateHelper(module, builder, "cf_http_end", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy()});
    auto *fSilentBegin = getOrCreateHelper(module, builder, "cf_silent_begin", builder.getPtrTy(), {builder.getPtrTy()});
    auto *fSilentEnd = getOrCreateHelper(module, builder, "cf_silent_end", builder.getVoidTy(), {});

    emitCall(builder, fBegin, {attrsVal});
    llvm::Value *discard = emitCall(builder, fSilentBegin, {out});

    WhitespaceState wsBody(ws.enabled, ws.flag);
    wsBody.markTag(false, false); // left neighbour is <cfhttp>
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

    emitCall(builder, fEnd, {cgi, server, cookie, application, session, url, form, variables});
    return nextIdx;
}

size_t compile_tag_transaction_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);

    auto *fBegin = getOrCreateHelper(module, builder, "cf_transaction_begin", builder.getVoidTy(), {builder.getPtrTy()});
    auto *fCommit = getOrCreateHelper(module, builder, "cf_transaction_commit", builder.getVoidTy(), {});
    auto *fRollback = getOrCreateHelper(module, builder, "cf_transaction_rollback", builder.getVoidTy(), {});

    // Parse the action attribute (default "begin").
    std::string action = "begin";
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        string aname(cfm_text + at.position, at.len);
        string anameLow = aname;
        anameLow.toLower();
        if (anameLow.equals("action")) {
            size_t vi = ai + 1;
            while (vi < attrParts->size() && isOperatorToken((*attrParts)[vi].token_id)) vi++;
            if (vi < attrParts->size() &&
                ((*attrParts)[vi].token_id == TextParser_cfml_DoubleString ||
                 (*attrParts)[vi].token_id == TextParser_cfml_SingleString)) {
                string raw(cfm_text + (*attrParts)[vi].position, (*attrParts)[vi].len);
                if (raw.length() >= 2) raw = raw.mid(1, raw.length() - 2);
                action = raw.constData();
                for (auto &c : action) c = static_cast<char>(tolower(c));
            }
            ai = vi;
            break;
        }
        ai++;
    }

    // Scan for the matching `</cftransaction>`, tracking nested `<cftransaction>`.
    std::vector<TextParserTokenItem> body;
    size_t bodyStart = tokens[start].position + tokens[start].len;
    size_t bodyEnd = 0;
    int depth = 0;
    size_t nextIdx = start + 1;
    bool foundEnd = false;
    if (startText.endsWith("/>")) {
        nextIdx = start + 1;
        foundEnd = true;
        bodyEnd = bodyStart;
    }
    for (size_t i = start + 1; !foundEnd && i < tokens.size(); i++) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("<cftransaction") && !tn.endsWith("/>")) depth++;
        } else if (tok.token_id == TextParser_cfml_EndTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("</cftransaction")) {
                if (depth > 0) depth--;
                else {
                    bodyEnd = tok.position;
                    nextIdx = i + 1;
                    foundEnd = true;
                    break;
                }
            }
        }
        body.push_back(tok);
    }
    if (!foundEnd) {
        bodyEnd = cfm_text_size;
        nextIdx = tokens.size();
    }

    if (action == "commit" || action == "rollback") {
        // No body: finish the current transaction immediately.
        emitCall(builder, action == "commit" ? fCommit : fRollback, {});
        return nextIdx;
    }
    // Default action "begin": wrap the body so it commits on success and rolls
    // back + re-raises on an exception (native EH, like cftry).
    emitCall(builder, fBegin, {llvm::ConstantPointerNull::get(builder.getPtrTy())});

    auto compileTagBody = [&](const std::vector<TextParserTokenItem> &bodyToks,
                              size_t bodyStartPos, size_t bodyEndPos) {
        WhitespaceState wsBody(ws.enabled, ws.flag);
        wsBody.markTag(false, false); // left neighbour is <cftransaction>
        size_t bodyPos = bodyStartPos;
        size_t bidx = 0;
        compile_token_list(bodyToks, bidx, bodyPos, context, module, builder, mainfunc,
                           out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                           cfm_text, bodyEndPos, loopStack);
        if (bodyPos < bodyEndPos) {
            wsBody.feed(module, builder, out, cfm_text + bodyPos, bodyEndPos - bodyPos, WsRight::Tag);
        }
        wsBody.finish(module, builder, out, WsRight::Tag);
    };

    // A catch-all clause: roll back the transaction and re-raise.
    std::vector<TryCatchClause> clauses;
    clauses.push_back({"any", ""});
    emit_try_catch_codegen(context, module, builder, mainfunc, out,
                           cgi, server, cookie, application, session, url, form, variables,
                           cfm_text, cfm_text_size, loopStack, clauses, false,
                           [&]() { compileTagBody(body, bodyStart, bodyEnd); },
                           [&](size_t, llvm::Value *captured) {
                               emitCall(builder, fRollback, {});
                               auto *fThrow = getOrCreateHelper(module, builder, "cf_eh_throw", builder.getVoidTy(), {builder.getPtrTy()});
                               emitCall(builder, fThrow, {captured});
                               builder.CreateUnreachable();
                               auto deadBB = llvm::BasicBlock::Create(context, "tx.catch.cont", mainfunc);
                               builder.SetInsertPoint(deadBB);
                           },
                           []() {});
    // The catch body re-raises, so the merge block after emit_try_catch_codegen
    // is only reached on success: commit the transaction there.
    emitCall(builder, fCommit, {});
    return nextIdx;
}

// <cfstoredproc procedure=".." datasource=".." ...> ... </cfstoredproc>: pushes
// a runtime call context (cf_storedproc_begin), compiles the body (whose
// <cfprocparam>/<cfprocresult> tags append parameters and result-set bindings
// to the context) into a discard buffer, then executes the procedure
// (cf_storedproc_end) which assigns the result sets / out values / status /
// execution-time variables. Attribute values compile like <cfquery> so
// `procedure="#name#"` evaluates.
size_t compile_tag_storedproc_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);

    // Parse the attributes (like <cfquery>): the valid CFSTOREDPROC set.
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    static const std::unordered_set<std::string> validAttrs = {
        "procedure", "datasource", "username", "password", "blockfactor",
        "debug", "returncode", "result"
    };

    auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
    auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                           {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
    auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});

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
            throw webstrada::exception("Attribute validation error for the cfstoredproc tag.", detail);
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

    // Scan for the matching `</cfstoredproc>` (tracking nested <cfstoredproc>).
    std::vector<TextParserTokenItem> body;
    size_t bodyStart = tokens[start].position + tokens[start].len;
    size_t bodyEnd = 0;
    int depth = 0;
    size_t nextIdx = start + 1;
    bool foundEnd = false;
    for (size_t i = start + 1; !foundEnd && i < tokens.size(); i++) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("<cfstoredproc") && !tn.endsWith("/>")) {
                depth++;
            }
        } else if (tok.token_id == TextParser_cfml_EndTag) {
            string tn(cfm_text + tok.position, tok.len);
            tn.toLower();
            if (tn.startWith("</cfstoredproc")) {
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
    if (!foundEnd) {
        bodyEnd = cfm_text_size;
        nextIdx = tokens.size();
    }

    // Push the call context + a discard buffer for the body, compile the body,
    // then execute the procedure.
    auto *fBegin = getOrCreateHelper(module, builder, "cf_storedproc_begin", builder.getPtrTy(), {});
    auto *fEnd = getOrCreateHelper(module, builder, "cf_storedproc_end", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy()});
    llvm::Value *discard = emitCall(builder, fBegin, {});

    WhitespaceState wsBody(ws.enabled, ws.flag);
    wsBody.markTag(false, false); // left neighbour is <cfstoredproc>
    size_t bodyPos = bodyStart;
    size_t bidx = 0;
    compile_token_list(body, bidx, bodyPos, context, module, builder, mainfunc,
                       discard, wsBody, cgi, server, cookie, application, session, url, form, variables,
                       cfm_text, bodyEnd, loopStack);
    if (bodyPos < bodyEnd) {
        wsBody.feed(module, builder, discard, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
    }
    wsBody.finish(module, builder, discard, WsRight::Tag);

    emitCall(builder, fEnd, {attrsVal, cgi, server, cookie, application, session, url, form, variables});
    return nextIdx;
}

// ---------------------------------------------------------------------------
// <cfdirectory> / <cffile> / <cfzip> / <cfzipparam> — file/directory/archive
// tags. The attribute struct is built once (lowercased keys) and passed to the
// runtime; unknown attributes are compile-time errors with CF's message, and a
// static (literal) ACTION value is validated at compile time like CF.
// ---------------------------------------------------------------------------

// Compile a single attribute value token list into an LLVM value.
static llvm::Value* compileTagAttrValue(
    llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Function *mainfunc,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie, llvm::Value *application,
    llvm::Value *session, llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const std::vector<TextParserTokenItem> &valToks, const char *cfm_text)
{
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
}

// Builds a cfvariant Struct of the tag's evaluated attributes under lowercased
// keys. Unknown attributes are compile-time errors with CF's message
// ("Attribute validation error for the <name> tag. The tag does not have an
// attribute called X. The valid attribute(s) are ..."). Returns the struct.
static llvm::Value* compileTagAttrsStruct(
    llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Function *mainfunc,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie, llvm::Value *application,
    llvm::Value *session, llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const std::vector<TextParserTokenItem> *attrParts, const char *cfm_text,
    const char *tagDisplay, const std::unordered_set<std::string> &validAttrs)
{
    auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
    auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                           {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
    llvm::Value *attrsVal = emitCall(builder, fCreateStruct, {});
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        string aname(cfm_text + at.position, at.len);
        string anameLow = aname;
        anameLow.toLower();
        if (validAttrs.find(anameLow.constData()) == validAttrs.end()) {
            std::string list;
            std::vector<std::string> sorted(validAttrs.begin(), validAttrs.end());
            std::sort(sorted.begin(), sorted.end());
            for (size_t i = 0; i < sorted.size(); i++) {
                if (i) list += ", ";
                list += sorted[i];
            }
            throw webstrada::exception(webstrada::string(("Attribute validation error for the " +
                                                        std::string(tagDisplay) + " tag.").c_str()),
                                      webstrada::string((std::string("The tag does not have an attribute called ") +
                                                        aname.constData() + ". The valid attribute(s) are " +
                                                        list + ".").c_str()));
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
        llvm::Value *val = compileTagAttrValue(module, builder, mainfunc,
                                               cgi, server, cookie, application, session, url, form, variables,
                                               valToks, cfm_text);
        if (val) {
            auto *keyStr = builder.CreateGlobalString(llvm::StringRef(anameLow.constData(), anameLow.length()), "", 0, module, true);
            auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
            emitCall(builder, fIndexAssign, {attrsVal,
                emitCall(builder, fCreateString, {keyStr}), val});
        }
        ai = vi;
    }
    return attrsVal;
}

// Extract a plain quoted-string literal (no # interpolation) from a value token
// list ("" returned when not a plain literal).
static bool tagStaticLiteral(const std::vector<TextParserTokenItem> &toks,
                             const char *cfm_text, std::string &out)
{
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
}

// Scan a tag for its matching end tag (tracking nested same-name start tags),
// filling `body` with the inner tokens and returning the next index.
static size_t scanTagBody(const std::vector<TextParserTokenItem> &tokens, size_t start,
                          const char *tagLower, const char *cfm_text,
                          std::vector<TextParserTokenItem> &body,
                          size_t &bodyStart, size_t &bodyEnd, size_t cfm_text_size)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);
    bodyStart = tokens[start].position + tokens[start].len;
    bodyEnd = 0;
    int depth = 0;
    size_t nextIdx = start + 1;
    bool foundEnd = false;
    std::string tagName = tagLower[0] == '<' ? tagLower + 1 : tagLower;
    std::string endTag = "</" + tagName;
    // Extract the exact tag name from a start/end token (case-insensitive),
    // so a name-prefixed sibling like <cfzipparam> never counts as a nested
    // <cfzip>.
    auto tagNameOfToken = [&](const TextParserTokenItem &tok) -> std::string {
        string raw(cfm_text + tok.position, tok.len);
        std::string s = raw.constData();
        size_t b = 0;
        if (s.rfind("</", 0) == 0) b = 2;
        else if (s.rfind("<", 0) == 0) b = 1;
        size_t e = b;
        while (e < s.length() && (isalnum((unsigned char)s[e]) || s[e] == '_')) e++;
        std::string name = s.substr(b, e - b);
        for (char &c : name) c = static_cast<char>(tolower((unsigned char)c));
        return name;
    };
    if (startText.endsWith("/>")) {
        nextIdx = start + 1;
        foundEnd = true;
        bodyEnd = bodyStart;
    }
    for (size_t i = start + 1; !foundEnd && i < tokens.size(); i++) {
        const auto &tok = tokens[i];
        if (tok.token_id == TextParser_cfml_StartTag) {
            if (tagNameOfToken(tok) == tagName) depth++;
        } else if (tok.token_id == TextParser_cfml_EndTag) {
            if (tagNameOfToken(tok) == tagName) {
                if (depth > 0) depth--;
                else {
                    bodyEnd = tok.position;
                    nextIdx = i + 1;
                    foundEnd = true;
                    break;
                }
            }
        }
        body.push_back(tok);
    }
    if (!foundEnd) {
        // No matching end tag: this is a single tag (`<cffile>`/`<cfzip>` need
        // no closing tag; only write/append-style bodies opt in with an
        // explicit `</cffile>`). Treat it as body-less instead of swallowing
        // the rest of the file as the tag body — the old behaviour compiled
        // everything after an un-closed `<cffile action="read">` into a dead
        // block, silently dropping the remainder of the template/CFC (was the
        // MangoBlog "PARENT" Preferences error).
        body.clear();
        bodyEnd = bodyStart;
        nextIdx = start + 1;
    }
    return nextIdx;
}

// <cfdirectory> — the runtime receives the attrs struct; the static ACTION
// value is validated at compile time (CF's error), and a missing directory
// attribute is a compile error for the non-list actions (CF rejects it).
size_t compile_tag_directory_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    (void)context; (void)out; (void)ws; (void)loopStack;
    static const std::unordered_set<std::string> dirValidAttrs = {
        "action", "recurse", "directory", "destination", "name", "filter", "sort",
        "newdirectory", "mode", "listinfo", "type", "storeLocation", "storeACL"
    };
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }

    // Static ACTION validation.
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        string aname(cfm_text + at.position, at.len);
        string anameLow = aname;
        anameLow.toLower();
        if (anameLow.equals("action")) {
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
            std::string lit;
            if (tagStaticLiteral(valToks, cfm_text, lit)) {
                std::string up;
                for (char c : lit) up += static_cast<char>(toupper((unsigned char)c));
                static const std::unordered_set<std::string> valid = {
                    "RENAME", "CREATE", "COPY", "LIST", "DELETE"};
                if (valid.find(up) == valid.end()) {
                    throw webstrada::exception(webstrada::string("Attribute validation error for CFDIRECTORY."),
                        webstrada::string(("The value of the ACTION attribute, which is currently " +
                                          lit + ", must be one of the values: RENAME,CREATE,COPY,LIST,DELETE.").c_str()));
                }
            }
            break;
        }
        ai++;
    }

    llvm::Value *attrsVal = compileTagAttrsStruct(module, builder, mainfunc,
                                                  cgi, server, cookie, application, session, url, form, variables,
                                                  attrParts, cfm_text, "directory", dirValidAttrs);
    auto *fTag = getOrCreateHelper(module, builder, "cf_directory_tag", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy()});
    emitCall(builder, fTag, {attrsVal, cgi, server, cookie, application, session, url, form, variables});
    return start + 1;
}

// <cfzipparam> — appends a parameter to the active <cfzip> context.
size_t compile_tag_zipparam_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    (void)context; (void)out; (void)ws; (void)loopStack;
    (void)cfm_text_size;
    static const std::unordered_set<std::string> paramValidAttrs = {
        "source", "content", "entrypath", "filter", "prefix", "recurse", "charset",
        "encryptionalgorithm", "password"
    };
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    // Collect the named values.
    struct ParamAttr { std::string name; std::vector<TextParserTokenItem> valToks; };
    std::vector<ParamAttr> params;
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        ParamAttr pa;
        string aname(cfm_text + at.position, at.len);
        string anameLow = aname;
        anameLow.toLower();
        if (paramValidAttrs.find(anameLow.constData()) == paramValidAttrs.end()) {
            std::string validList;
            std::vector<std::string> sorted(paramValidAttrs.begin(), paramValidAttrs.end());
            std::sort(sorted.begin(), sorted.end());
            for (size_t i = 0; i < sorted.size(); i++) { if (i) validList += ", "; validList += sorted[i]; }
            throw webstrada::exception(webstrada::string("Attribute validation error for the cfzipparam tag."),
                webstrada::string((std::string("The tag does not have an attribute called ") + aname.constData() +
                                  ". The valid attribute(s) are " + validList + ".").c_str()));
        }
        pa.name = anameLow.constData();
        size_t vi = ai + 1;
        while (vi < attrParts->size()) {
            const auto &vt = (*attrParts)[vi];
            bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                               vi + 1 < attrParts->size() &&
                               isOperatorToken((*attrParts)[vi + 1].token_id));
            if (nextIsAttr) break;
            if (!isOperatorToken(vt.token_id)) pa.valToks.push_back(vt);
            vi++;
        }
        params.push_back(std::move(pa));
        ai = vi;
    }

    // Compile-time attribute-combination validation (CF's CFZIPPARAM TLD):
    // each combination has a required and an optional set; the present set must
    // satisfy one of them. The possible-combinations text is reproduced exactly.
    if (!params.empty()) {
        struct Combo { const char* req[4]; const char* opt[7]; int reqN; int optN; };
        static const Combo combos[4] = {
            {{"content", "entrypath"}, {"charset", "encryptionalgorithm", "password"}, 2, 3},
            {{"source"}, {"encryptionalgorithm", "entrypath", "filter", "password", "prefix", "recurse"}, 1, 6},
            {{"filter"}, {"encryptionalgorithm", "entrypath", "password", "recurse"}, 1, 4},
            {{"entrypath"}, {"encryptionalgorithm", "filter", "password", "recurse"}, 1, 4}
        };
        auto hasName = [&](const char *n) {
            for (const auto &p : params) if (p.name == n) return true;
            return false;
        };
        bool ok = false;
        for (const auto &c : combos) {
            bool allReq = true;
            for (int i = 0; i < c.reqN; i++) if (!hasName(c.req[i])) { allReq = false; break; }
            if (!allReq) continue;
            bool noExtra = true;
            for (const auto &p : params) {
                bool inCombo = false;
                for (int i = 0; i < c.reqN; i++) if (p.name == c.req[i]) { inCombo = true; break; }
                if (!inCombo) for (int i = 0; i < c.optN; i++) if (p.name == c.opt[i]) { inCombo = true; break; }
                if (!inCombo) { noExtra = false; break; }
            }
            if (noExtra) { ok = true; break; }
        }
        if (!ok) {
            std::vector<std::string> present;
            for (const auto &p : params) {
                std::string up;
                for (char c : p.name) up += static_cast<char>(toupper((unsigned char)c));
                present.push_back(up);
            }
            std::sort(present.begin(), present.end());
            std::string presentList;
            for (size_t i = 0; i < present.size(); i++) { if (i) presentList += ","; presentList += present[i]; }
            throw webstrada::exception(webstrada::string("Attribute validation error for tag CFZIPPARAM."),
                webstrada::string(("It has an invalid attribute combination: " + presentList +
                    ". Possible combinations are: Required attributes: 'content,entrypath'. Optional attributes: "
                    "'charset,encryptionalgorithm,password'. Required attributes: 'source'. Optional attributes: "
                    "'encryptionalgorithm,entrypath,filter,password,prefix,recurse'. Required attributes: 'filter'. "
                    "Optional attributes: 'encryptionalgorithm,entrypath,password,recurse'. Required attributes: "
                    "'entrypath'. Optional attributes: 'encryptionalgorithm,filter,password,recurse'.").c_str()));
        }
    }

    llvm::Value *aSource = nullptr, *aContent = nullptr, *aEntrypath = nullptr,
                *aFilter = nullptr, *aPrefix = nullptr, *aRecurse = nullptr, *aCharset = nullptr,
                *aEncAlg = nullptr, *aPassword = nullptr;
    for (const auto &p : params) {
        llvm::Value *val = compileTagAttrValue(module, builder, mainfunc,
                                               cgi, server, cookie, application, session, url, form, variables,
                                               p.valToks, cfm_text);
        if (p.name == "source") aSource = val;
        else if (p.name == "content") aContent = val;
        else if (p.name == "entrypath") aEntrypath = val;
        else if (p.name == "filter") aFilter = val;
        else if (p.name == "prefix") aPrefix = val;
        else if (p.name == "recurse") aRecurse = val;
        else if (p.name == "charset") aCharset = val;
        else if (p.name == "encryptionalgorithm") aEncAlg = val;
        else if (p.name == "password") aPassword = val;
    }
    llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
    auto *fParam = getOrCreateHelper(module, builder, "cf_zip_param", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy()});
    emitCall(builder, fParam, {aSource ? aSource : nullPtr,
                               aContent ? aContent : nullPtr,
                               aEntrypath ? aEntrypath : nullPtr,
                               aFilter ? aFilter : nullPtr,
                               aPrefix ? aPrefix : nullPtr,
                               aRecurse ? aRecurse : nullPtr,
                               aCharset ? aCharset : nullPtr,
                               aEncAlg ? aEncAlg : nullPtr,
                               aPassword ? aPassword : nullPtr});
    return start + 1;
}

// <cfzip> — pushes the zip context, compiles the body (whose <cfzipparam>
// children append to the context) into a discard buffer, then executes the
// action via cf_zip_end. The static ACTION value is validated at compile time.
size_t compile_tag_zip_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    // CF 2025 validates <cfzip> attributes per ACTION at compile time: an
    // unknown attribute names the action's valid set, and a missing required
    // attribute is a compile-time error too.
    static const std::unordered_set<std::string> zipActionAttrs[6] = {
        // zip (default)
        {"action", "encryptionalgorithm", "file", "filter", "overwrite", "password",
         "prefix", "recurse", "source", "storepath"},
        // unzip
        {"action", "destination", "entrypath", "file", "filter", "maxunzipratio",
         "overwrite", "password", "recurse", "storepath"},
        // list
        {"action", "entrypath", "file", "filter", "name", "recurse", "showdirectory"},
        // read
        {"action", "charset", "entrypath", "file", "password", "variable"},
        // readBinary
        {"action", "entrypath", "file", "password", "variable"},
        // delete
        {"action", "entrypath", "file", "filter", "recurse"}
    };
    // required attributes per action (indices align with zipActionAttrs)
    static const char* zipRequiredAttrs[6][4] = {
        {"file"},
        {"destination", "file"},
        {"file", "name"},
        {"entrypath", "file"},
        {"entrypath", "file", "variable"},
        {"file"}
    };
    static const int zipRequiredCount[6] = {1, 2, 2, 2, 3, 1};
    static const std::unordered_set<std::string> zipAllAttrs = {
        "action", "charset", "destination", "encryptionalgorithm", "entrypath", "file",
        "filter", "maxunzipratio", "name", "overwrite", "password", "prefix", "recurse",
        "showdirectory", "source", "storepath", "variable"
    };

    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }

    // Collect the present attributes (lowercased names).
    struct ZipAttr { std::string name; std::vector<TextParserTokenItem> valToks; };
    std::vector<ZipAttr> zipAttrs;
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        ZipAttr za;
        string aname(cfm_text + at.position, at.len);
        string anameLow = aname;
        anameLow.toLower();
        za.name = anameLow.constData();
        size_t vi = ai + 1;
        while (vi < attrParts->size()) {
            const auto &vt = (*attrParts)[vi];
            bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                               vi + 1 < attrParts->size() &&
                               isOperatorToken((*attrParts)[vi + 1].token_id));
            if (nextIsAttr) break;
            if (!isOperatorToken(vt.token_id)) za.valToks.push_back(vt);
            vi++;
        }
        zipAttrs.push_back(std::move(za));
        ai = vi;
    }

    auto findZipAttr = [&](const char *n) -> const ZipAttr* {
        for (const auto &a : zipAttrs) if (a.name == n) return &a;
        return nullptr;
    };

    // Static ACTION value.
    std::string staticAction;
    bool hasStaticAction = false;
    if (const ZipAttr *a = findZipAttr("action")) {
        std::string lit;
        if (tagStaticLiteral(a->valToks, cfm_text, lit)) {
            hasStaticAction = true;
            for (char &c : lit) c = static_cast<char>(toupper((unsigned char)c));
            staticAction = lit;
            static const std::unordered_set<std::string> validActions = {
                "ZIP", "READ", "UNZIP", "LIST", "READBINARY", "DELETE"};
            if (validActions.find(staticAction) == validActions.end()) {
                throw webstrada::exception(webstrada::string("Attribute validation error for CFZIP."),
                    webstrada::string(("The value of the ACTION attribute, which is currently " +
                                      lit + ", must be one of the values: ZIP,READ,UNZIP,LIST,READBINARY,DELETE.").c_str()));
            }
        }
    }

    // Map the action to the valid/required sets.
    int actionIdx = 0;   // default zip
    if (hasStaticAction) {
        if (staticAction == "UNZIP") actionIdx = 1;
        else if (staticAction == "LIST") actionIdx = 2;
        else if (staticAction == "READ") actionIdx = 3;
        else if (staticAction == "READBINARY") actionIdx = 4;
        else if (staticAction == "DELETE") actionIdx = 5;
    }
    const std::unordered_set<std::string> &validSet =
        hasStaticAction ? zipActionAttrs[actionIdx] : zipAllAttrs;

    // Unknown attributes.
    std::vector<std::string> unknownAttrs;
    for (const auto &a : zipAttrs) {
        if (validSet.find(a.name) == validSet.end()) {
            std::string up;
            for (char c : a.name) up += static_cast<char>(toupper((unsigned char)c));
            unknownAttrs.push_back(up);
        }
    }
    if (!unknownAttrs.empty()) {
        std::sort(unknownAttrs.begin(), unknownAttrs.end());
        std::string list;
        for (size_t i = 0; i < unknownAttrs.size(); i++) { if (i) list += ","; list += unknownAttrs[i]; }
        std::string validList;
        std::vector<std::string> sortedValid(validSet.begin(), validSet.end());
        std::sort(sortedValid.begin(), sortedValid.end());
        for (size_t i = 0; i < sortedValid.size(); i++) {
            if (i) validList += ",";
            std::string up;
            for (char c : sortedValid[i]) up += static_cast<char>(toupper((unsigned char)c));
            validList += up;
        }
        throw webstrada::exception(webstrada::string("Attribute validation error for tag CFZIP."),
            webstrada::string(("It does not allow the attribute(s) " + list +
                              ". The valid attribute(s) are " + validList + ".").c_str()));
    }

    // Missing required attributes (only for a static action).
    if (hasStaticAction) {
        std::vector<std::string> missing;
        for (int ri = 0; ri < zipRequiredCount[actionIdx]; ri++) {
            const char *r = zipRequiredAttrs[actionIdx][ri];
            if (!findZipAttr(r)) missing.push_back(r);
        }
        if (!missing.empty()) {
            std::sort(missing.begin(), missing.end());
            std::string list;
            for (size_t i = 0; i < missing.size(); i++) { if (i) list += ","; list += missing[i]; }
            std::string up;
            for (char c : list) up += static_cast<char>(toupper((unsigned char)c));
            throw webstrada::exception(webstrada::string("Attribute validation error for tag CFZIP."),
                webstrada::string(("When the value of the ACTION attribute is " + staticAction +
                                  ", it requires the attribute(s): " + up + ".").c_str()));
        }
    }

    // Build the attribute struct (all present attributes are valid now).
    llvm::Value *attrsVal = compileTagAttrsStruct(module, builder, mainfunc,
                                                  cgi, server, cookie, application, session, url, form, variables,
                                                  attrParts, cfm_text, "zip", zipAllAttrs);

    // Scan for the matching </cfzip>.
    std::vector<TextParserTokenItem> body;
    size_t bodyStart = 0, bodyEnd = 0;
    size_t nextIdx = scanTagBody(tokens, start, "<cfzip", cfm_text, body, bodyStart, bodyEnd, cfm_text_size);

    // Push the context + a discard buffer for the body, compile the body, then
    // execute the action.
    auto *fBegin = getOrCreateHelper(module, builder, "cf_zip_begin", builder.getVoidTy(), {});
    auto *fEnd = getOrCreateHelper(module, builder, "cf_zip_end", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy()});
    auto *fSilentBegin = getOrCreateHelper(module, builder, "cf_silent_begin", builder.getPtrTy(), {builder.getPtrTy()});
    auto *fSilentEnd = getOrCreateHelper(module, builder, "cf_silent_end", builder.getVoidTy(), {});

    emitCall(builder, fBegin, {});
    llvm::Value *discard = emitCall(builder, fSilentBegin, {out});

    WhitespaceState wsBody(ws.enabled, ws.flag);
    wsBody.markTag(false, false);
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

    emitCall(builder, fEnd, {attrsVal, cgi, server, cookie, application, session, url, form, variables});
    return nextIdx;
}

// <cffile> — like the other file tags the attrs struct is passed to the
// runtime; additionally the write/append tag BODY is captured (CF's BodyTag
// doEndTag content) and handed to cf_file_tag. For a static non-write/append
// action the body is compiled into a dead block (CF's doStartTag returns
// SKIP_BODY) so a body syntax error still surfaces at compile time.
size_t compile_tag_file_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    static const std::unordered_set<std::string> fileValidAttrs = {
        "action", "block", "allowedextensions", "charset", "accept", "destination",
        "filefield", "nameconflict", "mode", "attributes", "source", "file",
        "variable", "output", "addnewline", "strict", "result", "continueonerror",
        "errors", "fixnewline"
    };
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }

    // Static ACTION validation + determine whether the body must be captured.
    std::string staticAction;
    bool hasStaticAction = false;
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        string aname(cfm_text + at.position, at.len);
        string anameLow = aname;
        anameLow.toLower();
        if (anameLow.equals("action")) {
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
            std::string lit;
            if (tagStaticLiteral(valToks, cfm_text, lit)) {
                hasStaticAction = true;
                staticAction = lit;
                std::string up;
                for (char c : lit) up += static_cast<char>(toupper((unsigned char)c));
                static const std::unordered_set<std::string> valid = {
                    "MOVE", "READ", "RENAME", "UPLOAD", "COPY", "UPLOADALL",
                    "READBINARY", "DELETE", "WRITE", "APPEND"};
                if (valid.find(up) == valid.end()) {
                    throw webstrada::exception(webstrada::string("Attribute validation error for CFFILE."),
                        webstrada::string(("The value of the ACTION attribute, which is currently " +
                                          lit + ", must be one of the values: MOVE,READ,RENAME,UPLOAD,COPY,"
                                          "UPLOADALL,READBINARY,DELETE,WRITE,APPEND.").c_str()));
                }
                staticAction = up;
            }
            break;
        }
        ai++;
    }

    llvm::Value *attrsVal = compileTagAttrsStruct(module, builder, mainfunc,
                                                  cgi, server, cookie, application, session, url, form, variables,
                                                  attrParts, cfm_text, "file", fileValidAttrs);

    // Scan for the matching </cffile>.
    std::vector<TextParserTokenItem> body;
    size_t bodyStart = 0, bodyEnd = 0;
    size_t nextIdx = scanTagBody(tokens, start, "<cffile", cfm_text, body, bodyStart, bodyEnd, cfm_text_size);

    llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
    bool wantsBody = !hasStaticAction || staticAction == "WRITE" || staticAction == "APPEND";
    bool hasBody = bodyEnd > bodyStart || !body.empty();

    if (wantsBody && hasBody) {
        // Capture the body output into a buffer and pass it to the runtime.
        auto *fSilentBegin = getOrCreateHelper(module, builder, "cf_silent_begin", builder.getPtrTy(), {builder.getPtrTy()});
        llvm::Value *capture = emitCall(builder, fSilentBegin, {out});
        WhitespaceState wsBody(ws.enabled, ws.flag);
        wsBody.markTag(false, false);
        size_t bodyPos = bodyStart;
        size_t bidx = 0;
        compile_token_list(body, bidx, bodyPos, context, module, builder, mainfunc,
                           capture, wsBody, cgi, server, cookie, application, session, url, form, variables,
                           cfm_text, bodyEnd, loopStack);
        if (bodyPos < bodyEnd) {
            wsBody.feed(module, builder, capture, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
        }
        wsBody.finish(module, builder, capture, WsRight::Tag);
        // NOTE: the capture buffer is left on the silent-buffer stack; cf_file_tag
        // copies its content and pops it (the same contract as cf_xml_end), so the
        // pointer stays valid until the runtime reads it.

        auto *fTag = getOrCreateHelper(module, builder, "cf_file_tag", builder.getVoidTy(),
            {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
             builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
             builder.getPtrTy(), builder.getPtrTy()});
        emitCall(builder, fTag, {attrsVal, cgi, server, cookie, application, session, url, form, variables, capture});
        return nextIdx;
    }

    // No body capture: compile any body into a dead block (never runs) so its
    // syntax still surfaces at compile time, then call the runtime with null.
    if (hasBody) {
        llvm::BasicBlock *deadBB = llvm::BasicBlock::Create(context, "cffile.body.dead", mainfunc);
        llvm::BasicBlock *contBB = llvm::BasicBlock::Create(context, "cffile.body.cont", mainfunc);
        builder.CreateBr(contBB);
        builder.SetInsertPoint(deadBB);
        WhitespaceState wsBody(ws.enabled, ws.flag);
        wsBody.markTag(false, false);
        size_t bodyPos = bodyStart;
        size_t bidx = 0;
        compile_token_list(body, bidx, bodyPos, context, module, builder, mainfunc,
                           out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                           cfm_text, bodyEnd, loopStack);
        if (bodyPos < bodyEnd) {
            wsBody.feed(module, builder, out, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
        }
        wsBody.finish(module, builder, out, WsRight::Tag);
        builder.CreateBr(contBB);
        builder.SetInsertPoint(contBB);
    }

    auto *fTag = getOrCreateHelper(module, builder, "cf_file_tag", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy()});
    emitCall(builder, fTag, {attrsVal, cgi, server, cookie, application, session, url, form, variables, nullPtr});
    return nextIdx;
}

// ---------------------------------------------------------------------------
// <cfcache> — fragment/whole-page template caching, client-cache headers,
// flush, and object-cache put/get.
// ---------------------------------------------------------------------------

// The cfcache attribute set (JSP TLD valid attributes), lowercased.
static bool isCacheAttr(const std::string &n)
{
    static const std::unordered_set<std::string> valid = {
        "usequerystring", "action", "directory", "timespan", "idletime",
        "expireurl", "username", "password", "protocol", "port", "id", "key",
        "region", "dependson", "usecache", "stripwhitespace", "value", "name",
        "metadata", "throwonerror"};
    return valid.find(n) != valid.end();
}

// Extract a plain quoted-string literal (no # interpolation) from a value
// token list. Used for compile-time attribute-value validation.
static bool cacheStaticLiteral(const std::vector<TextParserTokenItem> &toks,
                               const char *cfm_text, std::string &out)
{
    if (toks.size() != 1) return false;
    const auto &t = toks[0];
    if (t.token_id != TextParser_cfml_DoubleString &&
        t.token_id != TextParser_cfml_SingleString) {
        return false;
    }
    string raw(cfm_text + t.position, t.len);
    // Strip the surrounding quotes.
    std::string content;
    if (raw.length() >= 2) {
        content.assign(raw.constData() + 1, raw.length() - 2);
    }
    if (content.find('#') != std::string::npos) return false;
    out = content;
    return true;
}

static void throwCacheCompileError(const char *header, const std::string &detail)
{
    throw webstrada::exception(webstrada::string(header), webstrada::string(detail.c_str()));
}

size_t compile_tag_cache_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    string startText(cfm_text + tokens[start].position, tokens[start].len);
    bool selfCloseTag = startText.endsWith("/>");

    // ---- parse attributes ------------------------------------------------
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    struct CacheAttr {
        std::string name;
        std::string nameLow;
        std::vector<TextParserTokenItem> valToks;
    };
    std::vector<CacheAttr> attrs;
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        CacheAttr ca;
        ca.name.assign(cfm_text + at.position, at.len);
        ca.nameLow = ca.name;
        for (char &c : ca.nameLow) c = (char)tolower((unsigned char)c);
        size_t vi = ai + 1;
        while (vi < attrParts->size()) {
            const auto &vt = (*attrParts)[vi];
            bool nextIsAttr = (vt.token_id == TextParser_cfml_Variable &&
                               vi + 1 < attrParts->size() &&
                               isOperatorToken((*attrParts)[vi + 1].token_id));
            if (nextIsAttr) break;
            if (!isOperatorToken(vt.token_id)) ca.valToks.push_back(vt);
            vi++;
        }
        attrs.push_back(std::move(ca));
        ai = vi;
    }

    auto findAttr = [&](const char *n) -> const CacheAttr* {
        for (const auto &a : attrs) {
            if (a.nameLow == n) return &a;
        }
        return nullptr;
    };
    auto hasAttr = [&](const char *n) -> bool { return findAttr(n) != nullptr; };

    // ---- compile-time attribute validation (CF's TLD + static setters) ----
    std::string staticAction;
    bool hasStaticAction = false;
    if (const CacheAttr *a = findAttr("action")) {
        if (cacheStaticLiteral(a->valToks, cfm_text, staticAction)) {
            hasStaticAction = true;
        }
    }

    // 1. Bad action value (static). CF renders the value with its original
    // case ("... which is currently bogus ...").
    if (hasStaticAction) {
        static const std::unordered_set<std::string> validActions = {
            "CACHE", "CLIENTCACHE", "OPTIMAL", "FLUSH", "SERVERCACHE", "GET", "PUT"};
        std::string upper = staticAction;
        for (char &c : upper) c = (char)toupper((unsigned char)c);
        if (validActions.find(upper) == validActions.end()) {
            std::string shown = staticAction.empty() ? "''" : staticAction;
            throwCacheCompileError("Attribute validation error for CFCACHE.",
                "The value of the ACTION attribute, which is currently " + shown +
                ", must be one of the values: CACHE,CLIENTCACHE,OPTIMAL,FLUSH,SERVERCACHE,GET,PUT.");
        }
    }

    // 2. GET/PUT required attributes (static action).
    auto requiredList = [&](const char *action, const std::vector<std::string> &needed) {
        std::vector<std::string> missing;
        for (const auto &n : needed) {
            if (!hasAttr(n.c_str())) missing.push_back(n);
        }
        if (missing.empty()) return;
        std::sort(missing.begin(), missing.end());
        std::string list;
        for (size_t i = 0; i < missing.size(); i++) {
            if (i) list += ',';
            list += uppercase(missing[i]);
        }
        throwCacheCompileError("Attribute validation error for tag CFCACHE.",
            "When the value of the ACTION attribute is " + std::string(action) +
            ", it requires the attribute(s): " + list + ".");
    };
    if (hasStaticAction && uppercase(staticAction) == "GET") {
        requiredList("GET", {"id", "name"});
    }
    if (hasStaticAction && uppercase(staticAction) == "PUT") {
        requiredList("PUT", {"id", "value"});
    }

    // 3. FLUSH attribute combination (static action): id/expireurl are mutually
    // exclusive, and throwonerror requires id.
    if (hasStaticAction && uppercase(staticAction) == "FLUSH") {
        bool conflict = (hasAttr("id") && hasAttr("expireurl")) ||
                        (hasAttr("expireurl") && hasAttr("throwonerror"));
        if (conflict) {
            static const std::unordered_set<std::string> comboAttrs = {
                "action", "directory", "expireurl", "id", "idletime", "key",
                "password", "port", "protocol", "region", "throwonerror",
                "timespan", "usequerystring", "username"};
            std::vector<std::string> present;
            for (const auto &a : attrs) {
                if (comboAttrs.find(a.nameLow) != comboAttrs.end()) present.push_back(a.nameLow);
            }
            std::sort(present.begin(), present.end());
            std::string list;
            for (size_t i = 0; i < present.size(); i++) {
                if (i) list += ',';
                list += present[i];
            }
            throwCacheCompileError("Attribute validation error for tag CFCACHE.",
                "It has an invalid attribute combination: " + list +
                ". Possible combinations are:<li>Required attributes: None. Optional attributes: "
                "'action,directory,expireurl,idletime,key,password,port,protocol,region,timespan,usequerystring,username'. "
                "<li>Required attributes: None. Optional attributes: "
                "'action,directory,id,idletime,key,password,port,protocol,region,throwonerror,timespan,usequerystring,username'. ");
        }
    }

    // 4. Empty directory / dependson (static).
    auto emptyStringError = [&](const char *attrName) {
        throwCacheCompileError("Attribute validation error for CFCACHE.",
            "The value of the " + std::string(attrName) +
            " attribute is invalid. The length of the string, 0 character(s), "
            "must be greater than or equal to 1 character(s).");
    };
    if (const CacheAttr *a = findAttr("directory")) {
        std::string v;
        if (cacheStaticLiteral(a->valToks, cfm_text, v) && v.empty()) {
            emptyStringError("DIRECTORY");
        }
    }
    if (const CacheAttr *a = findAttr("dependson")) {
        std::string v;
        if (cacheStaticLiteral(a->valToks, cfm_text, v) && v.empty()) {
            emptyStringError("DEPENDSON");
        }
    }

    // 5. Unknown attributes (the first one, like CF).
    for (const auto &a : attrs) {
        if (!isCacheAttr(a.nameLow)) {
            throwCacheCompileError("Attribute validation error for the Cache tag.",
                "The tag does not have an attribute called " + a.name +
                ". The valid attribute(s) are useQueryString, action, directory, "
                "timespan, idleTime, expireURL, username, password, protocol, port, "
                "id, key, region, dependsOn, useCache, stripWhiteSpace, value, name, "
                "metadata, throwOnError.");
        }
    }

    // ---- compile attribute values ----------------------------------------
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

    llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
    llvm::Value *aAction = nullPtr, *aDirectory = nullPtr, *aTimespan = nullPtr;
    llvm::Value *aIdletime = nullPtr, *aExpireurl = nullPtr, *aUsername = nullPtr;
    llvm::Value *aPassword = nullPtr, *aProtocol = nullPtr, *aPort = nullPtr;
    llvm::Value *aId = nullPtr, *aKey = nullPtr, *aRegion = nullPtr;
    llvm::Value *aDependson = nullPtr, *aUsecache = nullPtr, *aStripwhitespace = nullPtr;
    llvm::Value *aValue = nullPtr, *aName = nullPtr, *aMetadata = nullPtr;
    llvm::Value *aThrowonerror = nullPtr, *aUsequerystring = nullPtr;
    for (const auto &a : attrs) {
        llvm::Value *val = compileValue(a.valToks);
        if (a.nameLow == "action") aAction = val;
        else if (a.nameLow == "directory") aDirectory = val;
        else if (a.nameLow == "timespan") aTimespan = val;
        else if (a.nameLow == "idletime") aIdletime = val;
        else if (a.nameLow == "expireurl") aExpireurl = val;
        else if (a.nameLow == "username") aUsername = val;
        else if (a.nameLow == "password") aPassword = val;
        else if (a.nameLow == "protocol") aProtocol = val;
        else if (a.nameLow == "port") aPort = val;
        else if (a.nameLow == "id") aId = val;
        else if (a.nameLow == "key") aKey = val;
        else if (a.nameLow == "region") aRegion = val;
        else if (a.nameLow == "dependson") aDependson = val;
        else if (a.nameLow == "usecache") aUsecache = val;
        else if (a.nameLow == "stripwhitespace") aStripwhitespace = val;
        else if (a.nameLow == "value") aValue = val;
        else if (a.nameLow == "name") aName = val;
        else if (a.nameLow == "metadata") aMetadata = val;
        else if (a.nameLow == "throwonerror") aThrowonerror = val;
        else if (a.nameLow == "usequerystring") aUsequerystring = val;
    }

    // ---- scan for the matching </cfcache> (body form) --------------------
    int depth = 0;
    size_t nextIdx = start + 1;
    bool hasEndTag = false;
    size_t bodyStart = tokens[start].position + tokens[start].len;
    size_t bodyEnd = 0;
    std::vector<TextParserTokenItem> body;
    if (!selfCloseTag) {
        bool foundEnd = false;
        for (size_t i = start + 1; i < tokens.size(); i++) {
            const auto &tok = tokens[i];
            if (tok.token_id == TextParser_cfml_StartTag) {
                string tn(cfm_text + tok.position, tok.len);
                tn.toLower();
                if (tn.startWith("<cfcache") && !tn.endsWith("/>")) {
                    depth++;
                }
            } else if (tok.token_id == TextParser_cfml_EndTag) {
                string tn(cfm_text + tok.position, tok.len);
                tn.toLower();
                if (tn.startWith("</cfcache")) {
                    if (depth > 0) {
                        depth--;
                    } else {
                        bodyEnd = tok.position;
                        nextIdx = i + 1;
                        hasEndTag = true;
                        foundEnd = true;
                        break;
                    }
                }
            }
            body.push_back(tok);
        }
        if (!foundEnd) {
            // A <cfcache> with no matching end tag is the whole-page (self
            // closing) form: the rest of the page continues after the tag.
            bodyEnd = bodyStart;
            nextIdx = start + 1;
        }
    }

    // ---- line number (for the fragment key, CF's "_line:N") --------------
    int lineNo = lineOfOffset(cfm_text, tokens[start].position);

    // ---- emit the begin call ---------------------------------------------
    std::vector<llvm::Type*> beginParams;
    for (int i = 0; i < 9; i++) beginParams.push_back(builder.getPtrTy());
    for (int i = 0; i < 20; i++) beginParams.push_back(builder.getPtrTy());
    beginParams.push_back(builder.getInt32Ty());
    beginParams.push_back(builder.getInt32Ty());
    auto *fBegin = getOrCreateHelper(module, builder, "cf_cache_tag_begin",
                                     builder.getPtrTy(), beginParams);
    llvm::Value *beginResult = emitCall(builder, fBegin,
        {out, cgi, server, cookie, application, session, url, form, variables,
         aAction, aDirectory, aTimespan, aIdletime, aExpireurl, aUsername, aPassword,
         aProtocol, aPort, aId, aKey, aRegion, aDependson, aUsecache, aStripwhitespace,
         aValue, aName, aMetadata, aThrowonerror, aUsequerystring,
         builder.getInt32(hasEndTag ? 1 : 0), builder.getInt32(lineNo)});

    auto *fTruthy = getOrCreateHelper(module, builder, "cfvariant_is_truthy",
                                      builder.getInt32Ty(), {builder.getPtrTy()});
    llvm::Value *truthy = emitCall(builder, fTruthy, {beginResult});
    llvm::Value *isHit = builder.CreateICmpNE(truthy, builder.getInt32(0));

    if (hasEndTag) {
        // Body form: on a hit (cached fragment served) skip the body and the
        // end-tag store; on a miss evaluate the body then store the fragment.
        auto *hitBB = llvm::BasicBlock::Create(context, "cfcache.hit", mainfunc);
        auto *missBB = llvm::BasicBlock::Create(context, "cfcache.miss", mainfunc);
        auto *endBB = llvm::BasicBlock::Create(context, "cfcache.end", mainfunc);
        builder.CreateCondBr(isHit, hitBB, missBB);

        builder.SetInsertPoint(hitBB);
        builder.CreateBr(endBB);

        builder.SetInsertPoint(missBB);
        if (bodyEnd > bodyStart || !body.empty()) {
            WhitespaceState wsBody(ws.enabled, ws.flag);
            wsBody.markTag(false, false); // left neighbour is <cfcache>
            size_t bodyPos = bodyStart;
            size_t bidx = 0;
            compile_token_list(body, bidx, bodyPos, context, module, builder, mainfunc,
                               out, wsBody, cgi, server, cookie, application, session, url, form, variables,
                               cfm_text, bodyEnd, loopStack);
            if (bodyPos < bodyEnd) {
                wsBody.feed(module, builder, out, cfm_text + bodyPos, bodyEnd - bodyPos, WsRight::Tag);
            }
            wsBody.finish(module, builder, out, WsRight::Tag);
        }
        auto *fEnd = getOrCreateHelper(module, builder, "cf_cache_tag_end",
                                       builder.getVoidTy(), {builder.getPtrTy(), builder.getPtrTy()});
        emitCall(builder, fEnd, {out, variables});
        builder.CreateBr(endBB);

        builder.SetInsertPoint(endBB);
        return nextIdx;
    }

    // Self-closing form: on a hit the whole page was served from cache, so the
    // rest of the page is skipped (CF's SKIP_PAGE).
    auto *hitBB = llvm::BasicBlock::Create(context, "cfcache.pagehit", mainfunc);
    auto *contBB = llvm::BasicBlock::Create(context, "cfcache.cont", mainfunc);
    builder.CreateCondBr(isHit, hitBB, contBB);
    builder.SetInsertPoint(hitBB);
    builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0, false));
    builder.SetInsertPoint(contBB);
    return nextIdx;
}

// ---------------------------------------------------------------------------
// <cfexecute> / <cffeed> / <cfwddx> — process execution, feed parsing/generation
// and WDDX serialization. cfexecute and cfwddx use the JSP-style attribute
// validation ("Attribute validation error for tag X. It does not allow the
// attribute(s) Y. The valid attribute(s) are ..."), verified byte-for-byte
// against CF 2025.
// ---------------------------------------------------------------------------

// Builds a cfvariant Struct of the tag's evaluated attributes under lowercased
// keys, validating against `validAttrs` with the JSP-style message:
// "Attribute validation error for tag {TAG}. It does not allow the
// attribute(s) {UNKNOWN_UPPER}. The valid attribute(s) are {UPPER_LIST}."
// (the valid list is sorted, comma-separated, uppercased — like CF 2025's
// TagLibraryInfo validation for cfexecute/cfwddx).
static llvm::Value* compileTagAttrsStructJsp(
    llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Function *mainfunc,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie, llvm::Value *application,
    llvm::Value *session, llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const std::vector<TextParserTokenItem> *attrParts, const char *cfm_text,
    const char *tagUpper, const std::unordered_set<std::string> &validAttrs)
{
    auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
    auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                           {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
    llvm::Value *attrsVal = emitCall(builder, fCreateStruct, {});
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        string aname(cfm_text + at.position, at.len);
        string anameLow = aname;
        anameLow.toLower();
        if (validAttrs.find(anameLow.constData()) == validAttrs.end()) {
            std::string list;
            std::vector<std::string> sorted(validAttrs.begin(), validAttrs.end());
            std::sort(sorted.begin(), sorted.end());
            for (size_t i = 0; i < sorted.size(); i++) {
                if (i) list += ",";
                std::string up = sorted[i];
                for (auto &c : up) c = static_cast<char>(toupper((unsigned char)c));
                list += up;
            }
            std::string unk = aname.constData() ? aname.constData() : "";
            for (auto &c : unk) c = static_cast<char>(toupper((unsigned char)c));
            throw webstrada::exception(webstrada::string(("Attribute validation error for tag " +
                                                        std::string(tagUpper) + ".").c_str()),
                                      webstrada::string((std::string("It does not allow the attribute(s) ") +
                                                        unk + ". The valid attribute(s) are " +
                                                        list + ".").c_str()));
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
        llvm::Value *val = compileTagAttrValue(module, builder, mainfunc,
                                               cgi, server, cookie, application, session, url, form, variables,
                                               valToks, cfm_text);
        if (val) {
            auto *keyStr = builder.CreateGlobalString(llvm::StringRef(anameLow.constData(), anameLow.length()), "", 0, module, true);
            auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
            emitCall(builder, fIndexAssign, {attrsVal,
                emitCall(builder, fCreateString, {keyStr}), val});
        }
        ai = vi;
    }
    return attrsVal;
}

// Builds a cfvariant Struct of the tag's evaluated attributes under lowercased
// keys, accepting ALL attributes (no whitelist). Used by tags that CF
// validates by handler (not TagLibraryInfo), where unknown attributes are
// accepted and ignored — <cfftp> and <cfschedule>.
static llvm::Value* compileTagAttrsStructAll(
    llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Function *mainfunc,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie, llvm::Value *application,
    llvm::Value *session, llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const std::vector<TextParserTokenItem> *attrParts, const char *cfm_text)
{
    auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
    auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                           {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
    llvm::Value *attrsVal = emitCall(builder, fCreateStruct, {});
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
        llvm::Value *val = compileTagAttrValue(module, builder, mainfunc,
                                               cgi, server, cookie, application, session, url, form, variables,
                                               valToks, cfm_text);
        if (val) {
            auto *keyStr = builder.CreateGlobalString(llvm::StringRef(anameLow.constData(), anameLow.length()), "", 0, module, true);
            auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
            emitCall(builder, fIndexAssign, {attrsVal,
                emitCall(builder, fCreateString, {keyStr}), val});
        }
        ai = vi;
    }
    return attrsVal;
}

// Helper: reads a named attribute's value token list from the attr parts.
static void collectAttrTokens(const std::vector<TextParserTokenItem> *attrParts,
                              const char *cfm_text, const std::string &target,
                              std::vector<TextParserTokenItem> &out)
{
    for (size_t ai = 0; ai < attrParts->size(); ) {
        const auto &at = (*attrParts)[ai];
        if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
        string aname(cfm_text + at.position, at.len);
        string anameLow = aname;
        anameLow.toLower();
        if (anameLow.constData() == target) {
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
            out = std::move(valToks);
            return;
        }
        ai++;
    }
}

// <cfexecute> — the runtime receives the attrs struct plus the page output
// buffer. name is required (compile-time), unknown attributes are compile-time
// errors, and a static timeout is passed through (runtime evaluates it).
size_t compile_tag_execute_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    (void)context; (void)ws; (void)cfm_text_size; (void)loopStack;
    static const std::unordered_set<std::string> execValidAttrs = {
        "name", "arguments", "outputfile", "variable", "timeout",
        "errorvariable", "errorfile"
    };
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    std::vector<TextParserTokenItem> nameToks;
    collectAttrTokens(attrParts, cfm_text, "name", nameToks);
    if (nameToks.empty()) {
        throw webstrada::exception(webstrada::string("Attribute validation error for tag CFEXECUTE."),
                                  webstrada::string("It requires the attribute(s): NAME."));
    }
    // CF's TLD types the timeout attribute as int, so a static literal that is
    // not an integer is a compile-time error ("Attribute validation error for
    // tag CFEXECUTE."). A dynamic value is validated at runtime.
    std::vector<TextParserTokenItem> timeoutToks;
    collectAttrTokens(attrParts, cfm_text, "timeout", timeoutToks);
    std::string litTimeout;
    if (tagStaticLiteral(timeoutToks, cfm_text, litTimeout)) {
        std::string t = litTimeout;
        size_t b = t.find_first_not_of(" \t\r\n");
        if (b != std::string::npos) t = t.substr(b);
        bool validInt = !t.empty() &&
            (t[0] == '-' || t[0] == '+' || std::isdigit(static_cast<unsigned char>(t[0])));
        if (validInt) {
            size_t s = (t[0] == '-' || t[0] == '+') ? 1 : 0;
            if (s >= t.size()) validInt = false;
            else for (size_t i = s; i < t.size() && validInt; i++) {
                if (!std::isdigit(static_cast<unsigned char>(t[i]))) validInt = false;
            }
        }
        if (!validInt) {
            throw webstrada::exception(webstrada::string("Attribute validation error for tag CFEXECUTE."),
                webstrada::string(("The value of the TIMEOUT attribute, which is currently " +
                                  litTimeout + ", must be an integer.").c_str()));
        }
    }
    llvm::Value *attrsVal = compileTagAttrsStructJsp(module, builder, mainfunc,
                                                     cgi, server, cookie, application, session, url, form, variables,
                                                     attrParts, cfm_text, "CFEXECUTE", execValidAttrs);
    auto *fTag = getOrCreateHelper(module, builder, "cf_execute_tag", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy()});
    emitCall(builder, fTag, {out, attrsVal, cgi, server, cookie, application, session, url, form, variables});
    return start + 1;
}

// <cffeed> — validates the attribute set at compile time and calls cf_feed_tag
// with the attrs struct + scopes. The feed package is not installed on the CF
// 2025 RDS host, so the attribute-combination rules cannot be byte-verified
// (see BUGS_CF.md); the valid attribute set is taken from the FeedTag TLD.
size_t compile_tag_feed_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    (void)out; (void)ws; (void)cfm_text_size; (void)loopStack;
    static const std::unordered_set<std::string> feedValidAttrs = {
        "action", "source", "name", "properties", "query", "columnmap",
        "outputfile", "overwrite", "xmlvar", "enclosuredir",
        "overwriteenclosure", "ignoreenclosureerror", "timeout", "useragent",
        "proxyserver", "proxyport", "proxyuser", "proxypassword", "escapechars"
    };
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    llvm::Value *attrsVal = compileTagAttrsStructJsp(module, builder, mainfunc,
                                                     cgi, server, cookie, application, session, url, form, variables,
                                                     attrParts, cfm_text, "CFFEED", feedValidAttrs);
    auto *fTag = getOrCreateHelper(module, builder, "cf_feed_tag", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy()});
    emitCall(builder, fTag, {attrsVal, cgi, server, cookie, application, session, url, form, variables});
    return start + 1;
}

// <cfwddx> — validates the attribute set per action value (verified against CF
// 2025): each action has its own valid attribute set and required attributes,
// so a static action value is validated at compile time with the per-action
// messages, while a dynamic action is passed through and validated at runtime.
size_t compile_tag_wddx_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    (void)context; (void)ws; (void)cfm_text_size; (void)loopStack;
    // The full TLD attribute set (per-action subsets are validated below).
    static const std::unordered_set<std::string> wddxAllAttrs = {
        "action", "input", "output", "usetimezoneinfo", "validate", "toplevelvariable"
    };
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    std::vector<TextParserTokenItem> actionToks;
    collectAttrTokens(attrParts, cfm_text, "action", actionToks);
    if (actionToks.empty()) {
        throw webstrada::exception(webstrada::string("Attribute validation error for the CFWDDX tag."),
                                  webstrada::string("The tag requires the ACTION attribute."));
    }

    std::string litAction;
    if (tagStaticLiteral(actionToks, cfm_text, litAction)) {
        std::string up;
        for (char c : litAction) up += static_cast<char>(toupper((unsigned char)c));
        static const std::unordered_set<std::string> validActions = {
            "CFML2JS", "CFML2WDDX", "WDDX2JS", "WDDX2CFML"};
        if (validActions.find(up) == validActions.end()) {
            throw webstrada::exception(webstrada::string("Attribute validation error for CFWDDX."),
                webstrada::string(("The value of the ACTION attribute, which is currently " +
                                  litAction + ", must be one of the values: CFML2JS,CFML2WDDX,WDDX2JS,WDDX2CFML.").c_str()));
        }
        // Per-action valid attribute set.
        std::unordered_set<std::string> validAttrs;
        if (up == "CFML2WDDX") {
            validAttrs = {"action", "input", "output", "usetimezoneinfo"};
        } else if (up == "WDDX2CFML") {
            validAttrs = {"action", "input", "output", "validate"};
        } else {
            validAttrs = {"action", "input", "output", "toplevelvariable"};
        }
        // Collect the present attribute names (lowercased).
        std::vector<std::string> present;
        for (size_t ai = 0; ai < attrParts->size(); ) {
            const auto &at = (*attrParts)[ai];
            if (at.token_id != TextParser_cfml_Variable) { ai++; continue; }
            string aname(cfm_text + at.position, at.len);
            string anameLow = aname;
            anameLow.toLower();
            present.push_back(anameLow.constData());
            ai++;
        }
        // Invalid attributes (present but not in the action's valid set), sorted.
        std::vector<std::string> invalid;
        for (const auto &p : present) {
            if (validAttrs.find(p) == validAttrs.end()) invalid.push_back(p);
        }
        if (!invalid.empty()) {
            std::sort(invalid.begin(), invalid.end());
            std::string list;
            for (size_t i = 0; i < invalid.size(); i++) {
                std::string u = invalid[i];
                for (auto &c : u) c = static_cast<char>(toupper((unsigned char)c));
                if (i) list += ",";
                list += u;
            }
            std::string validList;
            std::vector<std::string> sorted(validAttrs.begin(), validAttrs.end());
            std::sort(sorted.begin(), sorted.end());
            for (size_t i = 0; i < sorted.size(); i++) {
                std::string u = sorted[i];
                for (auto &c : u) c = static_cast<char>(toupper((unsigned char)c));
                if (i) validList += ",";
                validList += u;
            }
            throw webstrada::exception(webstrada::string("Attribute validation error for tag CFWDDX."),
                webstrada::string(("It does not allow the attribute(s) " + list +
                                  ". The valid attribute(s) are " + validList + ".").c_str()));
        }
        // Required attributes for the action (INPUT, plus OUTPUT for wddx2cfml
        // and TOPLEVELVARIABLE for the JS actions).
        std::vector<std::string> required;
        if (up == "WDDX2CFML") required = {"input", "output"};
        else if (up == "CFML2JS" || up == "WDDX2JS") required = {"input", "toplevelvariable"};
        else required = {"input"};
        std::vector<std::string> missing;
        for (const auto &r : required) {
            if (std::find(present.begin(), present.end(), r) == present.end()) missing.push_back(r);
        }
        if (!missing.empty()) {
            std::sort(missing.begin(), missing.end());
            std::string list;
            for (size_t i = 0; i < missing.size(); i++) {
                std::string u = missing[i];
                for (auto &c : u) c = static_cast<char>(toupper((unsigned char)c));
                if (i) list += ",";
                list += u;
            }
            throw webstrada::exception(webstrada::string("Attribute validation error for tag CFWDDX."),
                webstrada::string(("When the value of the ACTION attribute is " + up +
                                  ", it requires the attribute(s): " + list + ".").c_str()));
        }
        llvm::Value *attrsVal = compileTagAttrsStructJsp(module, builder, mainfunc,
                                                         cgi, server, cookie, application, session, url, form, variables,
                                                         attrParts, cfm_text, "CFWDDX", validAttrs);
        auto *fTag = getOrCreateHelper(module, builder, "cf_wddx_tag", builder.getVoidTy(),
            {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
             builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
             builder.getPtrTy(), builder.getPtrTy()});
        emitCall(builder, fTag, {out, attrsVal, cgi, server, cookie, application, session, url, form, variables});
        return start + 1;
    }

    // Dynamic action: no compile-time attribute/combination validation (CF
    // ignores non-TLD attributes and validates the TLD attributes at runtime).
    llvm::Value *attrsVal = compileTagAttrsStructJsp(module, builder, mainfunc,
                                                     cgi, server, cookie, application, session, url, form, variables,
                                                     attrParts, cfm_text, "CFWDDX", wddxAllAttrs);
    auto *fTag = getOrCreateHelper(module, builder, "cf_wddx_tag", builder.getVoidTy(),
        {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
         builder.getPtrTy(), builder.getPtrTy()});
    emitCall(builder, fTag, {out, attrsVal, cgi, server, cookie, application, session, url, form, variables});
    return start + 1;
}

// <cfftp> / <cfschedule> — not implemented: the runtime (cf_ftp_tag /
// cf_schedule_tag) only logs the call. Both tags are handler-validated in CF
// (NOT TagLibraryInfo-validated), so unknown attributes are accepted and
// ignored exactly like CF; only a missing `action` is rejected at compile
// time with CF's message. The evaluated attributes are collected into a
// struct (all of them) and the runtime logs the tag name + attributes.
// No FTP/scheduling work is performed.
size_t compile_tag_ftp_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    (void)context; (void)ws; (void)cfm_text_size; (void)loopStack;
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    std::vector<TextParserTokenItem> actionToks;
    collectAttrTokens(attrParts, cfm_text, "action", actionToks);
    if (actionToks.empty()) {
        throw webstrada::exception(webstrada::string("Attribute validation error for the CFFTP tag."),
                                  webstrada::string("The tag requires the ACTION attribute."));
    }
    llvm::Value *attrsVal = compileTagAttrsStructAll(module, builder, mainfunc,
                                                     cgi, server, cookie, application, session, url, form, variables,
                                                     attrParts, cfm_text);
    auto *fTag = getOrCreateHelper(module, builder, "cf_ftp_tag", builder.getVoidTy(),
        {builder.getPtrTy()});
    emitCall(builder, fTag, {attrsVal});
    return start + 1;
}

size_t compile_tag_schedule_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
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
    std::vector<LoopInfo> &loopStack)
{
    (void)context; (void)ws; (void)cfm_text_size; (void)loopStack;
    const std::vector<TextParserTokenItem> *attrParts = &tokens[start].children;
    for (const auto &ch : tokens[start].children) {
        if (ch.token_id == TextParser_cfml_Expression) {
            attrParts = &ch.children;
            break;
        }
    }
    std::vector<TextParserTokenItem> actionToks;
    collectAttrTokens(attrParts, cfm_text, "action", actionToks);
    if (actionToks.empty()) {
        throw webstrada::exception(webstrada::string("Attribute validation error for the CFSCHEDULE tag."),
                                  webstrada::string("The tag requires the ACTION attribute."));
    }
    llvm::Value *attrsVal = compileTagAttrsStructAll(module, builder, mainfunc,
                                                     cgi, server, cookie, application, session, url, form, variables,
                                                     attrParts, cfm_text);
    auto *fTag = getOrCreateHelper(module, builder, "cf_schedule_tag", builder.getVoidTy(),
        {builder.getPtrTy()});
    emitCall(builder, fTag, {attrsVal});
    return start + 1;
}

} // namespace webstrada
