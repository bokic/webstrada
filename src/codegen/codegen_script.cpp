/**
 * @file codegen_script.cpp
 * Code generation: script.
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

static size_t compile_script_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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

void compile_script_expression(
    const std::vector<TextParserTokenItem> &tokens,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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

// ---- definitions ----

static size_t compile_script_if_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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
    std::vector<LoopInfo> &loopStack
) {
    const auto &condToken = tokens[start + 1];

    llvm::Value *condVal = nullptr;
    if (!condToken.children.empty() && (condToken.children[0].token_id == TextParser_cfml_Expression || condToken.children[0].token_id == TextParser_cfml_ScriptExpression)) {
        condVal = CfmlExpressionToClang(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, condToken.children[0], cfm_text);
    } else {
        condVal = CfmlExpressionToClang(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, condToken, cfm_text);
    }
    auto *fTruthy = module->getFunction("cfvariant_is_truthy");
    if (!fTruthy) fTruthy = llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_is_truthy", module);
    auto *condBool = builder.CreateICmpNE(
        emitCall(builder, fTruthy, {condVal}),
        builder.getInt32(0)
    );

    auto *thenBB = llvm::BasicBlock::Create(context, "if.then", mainfunc);
    auto *elseBB = llvm::BasicBlock::Create(context, "if.else", mainfunc);
    auto *mergeBB = llvm::BasicBlock::Create(context, "if.merge", mainfunc);

    builder.CreateCondBr(condBool, thenBB, elseBB);

    // Compile the if body: either a { } CodeBlock or a single statement.
    builder.SetInsertPoint(thenBB);
    size_t bodyStart = start + 2;
    while (bodyStart < tokens.size() &&
           (tokens[bodyStart].token_id == TextParser_cfml_ScriptLineComment ||
            tokens[bodyStart].token_id == TextParser_cfml_ScriptBlockComment)) {
        bodyStart++;
    }
    size_t afterBody = bodyStart;
    if (bodyStart < tokens.size() && tokens[bodyStart].token_id == TextParser_cfml_CodeBlock) {
        const auto &bodyToken = tokens[bodyStart];
        if (!bodyToken.children.empty() && bodyToken.children[0].token_id == TextParser_cfml_ScriptExpression) {
            compile_script_expression(bodyToken.children[0].children, context, module, builder, mainfunc,
                                      out, cgi, server, cookie, application, session, url, form, variables,
                                      cfm_text, cfm_text_size, loopStack);
        }
        afterBody = bodyStart + 1;
    } else {
        afterBody = compile_script_statement(tokens, bodyStart, context, module, builder, mainfunc,
                                             out, cgi, server, cookie, application, session, url, form, variables,
                                             cfm_text, cfm_text_size, loopStack);
    }
    builder.CreateBr(mergeBB);

    // Compile the else/else-if bodies, if present. A chain of `else if`
    // branches is consumed by the nested compile_script_if_statement call; a
    // plain `else <body>` terminates the chain (dangling-else binding).
    builder.SetInsertPoint(elseBB);
    size_t nextIdx = afterBody;
    while (nextIdx < tokens.size() &&
           (tokens[nextIdx].token_id == TextParser_cfml_ScriptLineComment ||
            tokens[nextIdx].token_id == TextParser_cfml_ScriptBlockComment)) {
        nextIdx++;
    }
    if (nextIdx < tokens.size() &&
        tokens[nextIdx].token_id == TextParser_cfml_Keyword &&
        string(cfm_text + tokens[nextIdx].position, tokens[nextIdx].len).equals("else")) {
        size_t elseBodyStart = nextIdx + 1;
        while (elseBodyStart < tokens.size() &&
               (tokens[elseBodyStart].token_id == TextParser_cfml_ScriptLineComment ||
                tokens[elseBodyStart].token_id == TextParser_cfml_ScriptBlockComment)) {
            elseBodyStart++;
        }
        if (elseBodyStart < tokens.size() &&
            tokens[elseBodyStart].token_id == TextParser_cfml_Keyword &&
            string(cfm_text + tokens[elseBodyStart].position, tokens[elseBodyStart].len).equals("if")) {
            // else if (...) — the nested if consumes the rest of the chain.
            nextIdx = compile_script_if_statement(tokens, elseBodyStart, context, module, builder, mainfunc,
                                                  out, cgi, server, cookie, application, session, url, form, variables,
                                                  cfm_text, cfm_text_size, loopStack);
        } else if (elseBodyStart < tokens.size() &&
                   tokens[elseBodyStart].token_id == TextParser_cfml_CodeBlock) {
            const auto &elseBlock = tokens[elseBodyStart];
            if (!elseBlock.children.empty() && elseBlock.children[0].token_id == TextParser_cfml_ScriptExpression) {
                compile_script_expression(elseBlock.children[0].children, context, module, builder, mainfunc,
                                          out, cgi, server, cookie, application, session, url, form, variables,
                                          cfm_text, cfm_text_size, loopStack);
            }
            nextIdx = elseBodyStart + 1;
        } else {
            nextIdx = compile_script_statement(tokens, elseBodyStart, context, module, builder, mainfunc,
                                               out, cgi, server, cookie, application, session, url, form, variables,
                                               cfm_text, cfm_text_size, loopStack);
        }
    }
    builder.CreateBr(mergeBB);
    builder.SetInsertPoint(mergeBB);

    return nextIdx;
}

static llvm::Value *compile_script_expr_token(
    const TextParserTokenItem &exprTok,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *cgi,
    llvm::Value *server,
    llvm::Value *cookie,
    llvm::Value *application,
    llvm::Value *session,
    llvm::Value *url,
    llvm::Value *form,
    llvm::Value *variables,
    const char *cfm_text)
{
    llvm::Value *val = nullptr;
    if (!exprTok.children.empty() &&
        (exprTok.children[0].token_id == TextParser_cfml_Expression ||
         exprTok.children[0].token_id == TextParser_cfml_ScriptExpression)) {
        val = CfmlExpressionToClang(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, exprTok.children[0], cfm_text);
    } else {
        val = CfmlExpressionToClang(module, builder, mainfunc, cgi, server, cookie, application, session, url, form, variables, exprTok, cfm_text);
    }
    return val;
}

static llvm::Value *compile_script_condition_bool(
    const TextParserTokenItem &condTok,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *cgi,
    llvm::Value *server,
    llvm::Value *cookie,
    llvm::Value *application,
    llvm::Value *session,
    llvm::Value *url,
    llvm::Value *form,
    llvm::Value *variables,
    const char *cfm_text)
{
    auto *condVal = compile_script_expr_token(condTok, module, builder, mainfunc,
                                              cgi, server, cookie, application, session, url, form, variables, cfm_text);
    auto *fTruthy = module->getFunction("cfvariant_is_truthy");
    if (!fTruthy) fTruthy = llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_is_truthy", module);
    return builder.CreateICmpNE(emitCall(builder, fTruthy, {condVal}), builder.getInt32(0));
}

static size_t compile_script_body(
    const std::vector<TextParserTokenItem> &tokens,
    size_t bodyStart,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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
    while (bodyStart < tokens.size() &&
           (tokens[bodyStart].token_id == TextParser_cfml_ScriptLineComment ||
            tokens[bodyStart].token_id == TextParser_cfml_ScriptBlockComment)) {
        bodyStart++;
    }
    if (bodyStart < tokens.size() && tokens[bodyStart].token_id == TextParser_cfml_CodeBlock) {
        const auto &bodyToken = tokens[bodyStart];
        if (!bodyToken.children.empty() && bodyToken.children[0].token_id == TextParser_cfml_ScriptExpression) {
            compile_script_expression(bodyToken.children[0].children, context, module, builder, mainfunc,
                                      out, cgi, server, cookie, application, session, url, form, variables,
                                      cfm_text, cfm_text_size, loopStack);
        }
        return bodyStart + 1;
    }
    return compile_script_statement(tokens, bodyStart, context, module, builder, mainfunc,
                                    out, cgi, server, cookie, application, session, url, form, variables,
                                    cfm_text, cfm_text_size, loopStack);
}

static size_t compile_script_while(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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
    const auto &condTok = tokens[start + 1];
    size_t bodyStart = start + 2;

    auto *condBB = llvm::BasicBlock::Create(context, "while.cond", mainfunc);
    auto *bodyBB = llvm::BasicBlock::Create(context, "while.body", mainfunc);
    auto *endBB = llvm::BasicBlock::Create(context, "while.end", mainfunc);

    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);
    auto *condBool = compile_script_condition_bool(condTok, module, builder, mainfunc,
                                                   cgi, server, cookie, application, session, url, form, variables, cfm_text);
    builder.CreateCondBr(condBool, bodyBB, endBB);

    builder.SetInsertPoint(bodyBB);
    loopStack.push_back({condBB, endBB, "", nullptr, false});
    size_t afterBody = compile_script_body(tokens, bodyStart, context, module, builder, mainfunc,
                                           out, cgi, server, cookie, application, session, url, form, variables,
                                           cfm_text, cfm_text_size, loopStack);
    loopStack.pop_back();
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(condBB);
    }
    builder.SetInsertPoint(endBB);
    return afterBody;
}

static size_t compile_script_do_while(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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
    size_t bodyStart = start + 1;
    auto *bodyBB = llvm::BasicBlock::Create(context, "dowhile.body", mainfunc);
    auto *condBB = llvm::BasicBlock::Create(context, "dowhile.cond", mainfunc);
    auto *endBB = llvm::BasicBlock::Create(context, "dowhile.end", mainfunc);

    builder.CreateBr(bodyBB);
    builder.SetInsertPoint(bodyBB);
    loopStack.push_back({condBB, endBB, "", nullptr, false});
    size_t afterBody = compile_script_body(tokens, bodyStart, context, module, builder, mainfunc,
                                           out, cgi, server, cookie, application, session, url, form, variables,
                                           cfm_text, cfm_text_size, loopStack);
    loopStack.pop_back();
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(condBB);
    }

    // Locate the trailing `while (cond)`; afterBody points at the Function while.
    size_t whileIdx = afterBody;
    while (whileIdx < tokens.size() &&
           !(tokens[whileIdx].token_id == TextParser_cfml_Function &&
             string(cfm_text + tokens[whileIdx].position, tokens[whileIdx].len).equals("while"))) {
        whileIdx++;
    }
    if (whileIdx + 1 >= tokens.size() || tokens[whileIdx + 1].token_id != TextParser_cfml_Parenthesis) {
        throw webstrada::exception("Invalid do-while loop: expected while (condition)");
    }

    builder.SetInsertPoint(condBB);
    auto *condBool = compile_script_condition_bool(tokens[whileIdx + 1], module, builder, mainfunc,
                                                   cgi, server, cookie, application, session, url, form, variables, cfm_text);
    builder.CreateCondBr(condBool, bodyBB, endBB);
    builder.SetInsertPoint(endBB);
    return whileIdx + 2; // after the condition Parenthesis (the trailing `;` remains)
}

static size_t compile_script_for(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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
    const auto &forParen = tokens[start + 1];
    size_t bodyStart = start + 2;

    std::vector<TextParserTokenItem> innerTokens;
    if (!forParen.children.empty() &&
        (forParen.children[0].token_id == TextParser_cfml_Expression ||
         forParen.children[0].token_id == TextParser_cfml_ScriptExpression)) {
        innerTokens = forParen.children[0].children;
    } else {
        for (const auto &c : forParen.children) innerTokens.push_back(c);
    }

    // Split the loop header on top-level ExpressionEnd tokens. A classic for
    // always has at least two `;` separators (init; cond; incr).
    std::vector<std::vector<TextParserTokenItem>> parts;
    std::vector<TextParserTokenItem> cur;
    bool hasSemi = false;
    for (const auto &t : innerTokens) {
        if (t.token_id == TextParser_cfml_ExpressionEnd) {
            parts.push_back(cur);
            cur.clear();
            hasSemi = true;
        } else {
            cur.push_back(t);
        }
    }
    parts.push_back(cur);

    auto compilePart = [&](const std::vector<TextParserTokenItem> &part) -> llvm::Value* {
        if (part.empty()) return nullptr;
        TextParserTokenItem exprTok;
        exprTok.token_id = TextParser_cfml_Expression;
        exprTok.children = part;
        if (!part.empty()) {
            exprTok.position = part.front().position;
            exprTok.len = part.back().position + part.back().len - part.front().position;
        }
        return compile_script_expr_token(exprTok, module, builder, mainfunc,
                                         cgi, server, cookie, application, session, url, form, variables, cfm_text);
    };

    // for-in form: `for (item in collection)`
    if (!hasSemi) {
        size_t inIdx = innerTokens.size();
        for (size_t j = 0; j < innerTokens.size(); j++) {
            if (innerTokens[j].token_id == TextParser_cfml_Variable &&
                string(cfm_text + innerTokens[j].position, innerTokens[j].len).equals("in")) {
                inIdx = j;
                break;
            }
        }
        if (inIdx == innerTokens.size()) {
            throw webstrada::exception("Invalid for loop: expected ';' or 'in' in the loop header");
        }
        if (inIdx == 0 || inIdx + 1 >= innerTokens.size()) {
            throw webstrada::exception("Invalid for-in loop");
        }

        webstrada::string itemName(cfm_text + innerTokens[0].position, innerTokens[0].len);
        std::vector<TextParserTokenItem> collToks(innerTokens.begin() + inIdx + 1, innerTokens.end());
        TextParserTokenItem collTok;
        collTok.token_id = TextParser_cfml_Expression;
        collTok.children = collToks;
        if (!collToks.empty()) {
            collTok.position = collToks.front().position;
            collTok.len = collToks.back().position + collToks.back().len - collToks.front().position;
        }
        auto *collVal = compile_script_expr_token(collTok, module, builder, mainfunc,
                                                  cgi, server, cookie, application, session, url, form, variables, cfm_text);

        auto *fLen = module->getFunction("cfforin_length");
        if (!fLen) fLen = llvm::Function::Create(llvm::FunctionType::get(builder.getInt64Ty(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfforin_length", module);
        auto *fItem = module->getFunction("cfforin_item");
        if (!fItem) fItem = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getInt64Ty(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfforin_item", module);
        auto *fIndexAssign = module->getFunction("cfvariant_index_assign");
        if (!fIndexAssign) fIndexAssign = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index_assign", module);
        auto *fStr = module->getFunction("cfvariant_create_string");
        if (!fStr) fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
        auto *delimsVal = emitCall(builder, fStr, {builder.CreateGlobalString(",", "", 0, module, true)});

        auto *lenAlloca = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
        auto *idxAlloca = createEntryAlloca(builder, mainfunc, builder.getInt64Ty());
        builder.CreateStore(emitCall(builder, fLen, {collVal, delimsVal}), lenAlloca);
        builder.CreateStore(builder.getInt64(1), idxAlloca);

    auto *condBB = llvm::BasicBlock::Create(context, "forin.cond", mainfunc);
    auto *bodyBB = llvm::BasicBlock::Create(context, "forin.body", mainfunc);
    auto *incBB = llvm::BasicBlock::Create(context, "forin.inc", mainfunc);
    auto *endBB = llvm::BasicBlock::Create(context, "forin.end", mainfunc);

    // Per-iteration temp cleanup: expression temporaries created by the loop
    // condition / body / increment (struct literals, comparison results, ...)
    // are freed at the end of each iteration instead of living until the
    // enclosing function returns (was BUGS.md "Temp-variant accumulation").
    // Payloads assigned to variables are refcounted, so nothing referenced
    // dangles.
    auto *fLoopSave = getOrCreateHelper(module, builder, "cfvariant_cleanup_save", builder.getInt64Ty(), {});
    auto *fLoopRestore = getOrCreateHelper(module, builder, "cfvariant_cleanup_restore", builder.getVoidTy(),
                                            {builder.getInt64Ty()});

    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);
    llvm::Value *iterSave = builder.CreateCall(fLoopSave, {});
    auto *idxCur = builder.CreateLoad(builder.getInt64Ty(), idxAlloca);
    auto *lenCur = builder.CreateLoad(builder.getInt64Ty(), lenAlloca);
    auto *done = builder.CreateICmpSGT(idxCur, lenCur);
    builder.CreateCondBr(done, endBB, bodyBB);

    builder.SetInsertPoint(bodyBB);
    auto *itemVal = emitCall(builder, fItem, {collVal, idxCur, delimsVal});
    auto *itemKey = emitCall(builder, fStr, {builder.CreateGlobalString(llvm::StringRef(itemName.constData(), itemName.length()), "", 0, module, true)});
    emitCall(builder, fIndexAssign, {variables, itemKey, itemVal});

    loopStack.push_back({incBB, endBB, itemName, idxAlloca, false});
    size_t afterBody = compile_script_body(tokens, bodyStart, context, module, builder, mainfunc,
                                           out, cgi, server, cookie, application, session, url, form, variables,
                                           cfm_text, cfm_text_size, loopStack);
    loopStack.pop_back();
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(incBB);
    }

    builder.SetInsertPoint(incBB);
    auto *idx2 = builder.CreateLoad(builder.getInt64Ty(), idxAlloca);
    builder.CreateStore(builder.CreateAdd(idx2, builder.getInt64(1)), idxAlloca);
    emitCall(builder, fLoopRestore, {iterSave});
    builder.CreateBr(condBB);

    builder.SetInsertPoint(endBB);
    emitCall(builder, fLoopRestore, {iterSave});
    return afterBody;
    }

    // Classic for: init; cond; incr
    auto *condBB = llvm::BasicBlock::Create(context, "for.cond", mainfunc);
    auto *bodyBB = llvm::BasicBlock::Create(context, "for.body", mainfunc);
    auto *incBB = llvm::BasicBlock::Create(context, "for.inc", mainfunc);
    auto *endBB = llvm::BasicBlock::Create(context, "for.end", mainfunc);

    // Per-iteration temp cleanup (see the for-in form above).
    auto *fLoopSave = getOrCreateHelper(module, builder, "cfvariant_cleanup_save", builder.getInt64Ty(), {});
    auto *fLoopRestore = getOrCreateHelper(module, builder, "cfvariant_cleanup_restore", builder.getVoidTy(),
                                            {builder.getInt64Ty()});

    if (parts.size() >= 1) compilePart(parts[0]); // init

    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);
    llvm::Value *iterSave = builder.CreateCall(fLoopSave, {});
    if (parts.size() >= 2 && !parts[1].empty()) {
        auto *condVal = compilePart(parts[1]);
        auto *fTruthy = module->getFunction("cfvariant_is_truthy");
        if (!fTruthy) fTruthy = llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_is_truthy", module);
        auto *condBool = builder.CreateICmpNE(emitCall(builder, fTruthy, {condVal}), builder.getInt32(0));
        builder.CreateCondBr(condBool, bodyBB, endBB);
    } else {
        builder.CreateBr(bodyBB);
    }

    builder.SetInsertPoint(bodyBB);
    loopStack.push_back({incBB, endBB, "", nullptr, false});
    size_t afterBody = compile_script_body(tokens, bodyStart, context, module, builder, mainfunc,
                                           out, cgi, server, cookie, application, session, url, form, variables,
                                           cfm_text, cfm_text_size, loopStack);
    loopStack.pop_back();
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(incBB);
    }

    builder.SetInsertPoint(incBB);
    if (parts.size() >= 3) compilePart(parts[2]); // incr
    emitCall(builder, fLoopRestore, {iterSave});
    builder.CreateBr(condBB);

    builder.SetInsertPoint(endBB);
    emitCall(builder, fLoopRestore, {iterSave});
    return afterBody;
}

static size_t compile_script_switch(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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
    auto *switchVal = compile_script_expr_token(tokens[start + 1], module, builder, mainfunc,
                                                cgi, server, cookie, application, session, url, form, variables, cfm_text);

    size_t bodyStart = start + 2;
    while (bodyStart < tokens.size() &&
           (tokens[bodyStart].token_id == TextParser_cfml_ScriptLineComment ||
            tokens[bodyStart].token_id == TextParser_cfml_ScriptBlockComment)) {
        bodyStart++;
    }
    if (bodyStart >= tokens.size() || tokens[bodyStart].token_id != TextParser_cfml_CodeBlock) {
        throw webstrada::exception("Invalid switch statement: expected { } block");
    }

    std::vector<TextParserTokenItem> bodyTokens;
    const auto &blockToken = tokens[bodyStart];
    if (!blockToken.children.empty() && blockToken.children[0].token_id == TextParser_cfml_ScriptExpression) {
        bodyTokens = blockToken.children[0].children;
    }

    struct CaseSeg {
        bool isDefault;
        std::vector<TextParserTokenItem> valueTokens;
        std::vector<TextParserTokenItem> bodyTokens;
    };
    std::vector<CaseSeg> segments;

    size_t idx = 0;
    while (idx < bodyTokens.size()) {
        const auto &tok = bodyTokens[idx];
        if (tok.token_id == TextParser_cfml_Variable) {
            webstrada::string vtext(cfm_text + tok.position, tok.len);
            if (vtext.equals("case") || vtext.equals("default")) {
                bool isDefault = vtext.equals("default");
                CaseSeg seg;
                seg.isDefault = isDefault;
                idx++;
                if (isDefault) {
                    if (idx < bodyTokens.size() &&
                        isOperatorToken(bodyTokens[idx].token_id) &&
                        string(cfm_text + bodyTokens[idx].position, bodyTokens[idx].len).equals(":")) {
                        idx++;
                    }
                } else {
                    while (idx < bodyTokens.size() &&
                           !(isOperatorToken(bodyTokens[idx].token_id) &&
                             string(cfm_text + bodyTokens[idx].position, bodyTokens[idx].len).equals(":"))) {
                        seg.valueTokens.push_back(bodyTokens[idx]);
                        idx++;
                    }
                    if (idx < bodyTokens.size()) idx++; // skip ':'
                }
                segments.push_back(seg);
                continue;
            }
        }
        if (segments.empty()) {
            throw webstrada::exception("Unexpected token in switch block (expected case or default)");
        }
        segments.back().bodyTokens.push_back(tok);
        idx++;
    }

    auto *endBB = llvm::BasicBlock::Create(context, "switch.end", mainfunc);
    std::vector<llvm::BasicBlock*> bodyBBs;
    llvm::BasicBlock *defaultBB = nullptr;
    std::vector<int> segToBodyBB(segments.size(), -1);
    size_t nonDefaultCount = 0;
    for (size_t p = 0; p < segments.size(); p++) {
        if (segments[p].isDefault) {
            defaultBB = llvm::BasicBlock::Create(context, "switch.default", mainfunc);
        } else {
            segToBodyBB[p] = static_cast<int>(bodyBBs.size());
            bodyBBs.push_back(llvm::BasicBlock::Create(context, ("switch.body" + std::to_string(bodyBBs.size())).c_str(), mainfunc));
            nonDefaultCount++;
        }
    }

    // Comparison chain: switchVal == caseValue → body block, else the next
    // check; a last non-match reaches the default body (or the switch end).
    auto *compareFunc = module->getFunction("cfvariant_compare");
    if (!compareFunc) compareFunc = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_compare", module);
    auto *isTruthyFunc = module->getFunction("cfvariant_is_truthy");
    if (!isTruthyFunc) isTruthyFunc = llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_is_truthy", module);

    std::vector<llvm::BasicBlock*> checkBBs;
    for (size_t bi = 0; bi < nonDefaultCount; bi++) {
        checkBBs.push_back(llvm::BasicBlock::Create(context, ("switch.check" + std::to_string(bi)).c_str(), mainfunc));
    }

    // The switch's entry block branches to the first check; each check branches
    // to its matching body block, and a non-match falls through to the next
    // check. The last check's non-match target is the default body (or end).
    llvm::BasicBlock *entryBB = builder.GetInsertBlock();
    builder.SetInsertPoint(entryBB);
    if (nonDefaultCount > 0) {
        if (!entryBB->getTerminator()) {
            builder.CreateBr(checkBBs[0]);
        }
    } else if (!entryBB->getTerminator()) {
        builder.CreateBr(defaultBB ? defaultBB : endBB);
    }

    size_t bodyBBIdx = 0;
    for (size_t cIdx = 0; cIdx < segments.size(); cIdx++) {
        const auto &seg = segments[cIdx];
        if (seg.isDefault) continue;

        builder.SetInsertPoint(checkBBs[bodyBBIdx]);

        TextParserTokenItem valueTok;
        valueTok.token_id = TextParser_cfml_Expression;
        valueTok.children = seg.valueTokens;
        if (!seg.valueTokens.empty()) {
            valueTok.position = seg.valueTokens.front().position;
            valueTok.len = seg.valueTokens.back().position + seg.valueTokens.back().len - seg.valueTokens.front().position;
        }
        auto *caseVal = compile_script_expr_token(valueTok, module, builder, mainfunc,
                                                  cgi, server, cookie, application, session, url, form, variables, cfm_text);
        auto *cmpResult = emitCall(builder, compareFunc, {switchVal, caseVal, builder.CreateGlobalString("EQ", "", 0, module, true)});
        auto *isCmpTrue = emitCall(builder, isTruthyFunc, {cmpResult});
        auto *matched = builder.CreateICmpNE(isCmpTrue, builder.getInt32(0));

        auto *targetBB = bodyBBs[bodyBBIdx];
        llvm::BasicBlock *nextBB = (bodyBBIdx + 1 < nonDefaultCount) ? checkBBs[bodyBBIdx + 1] : (defaultBB ? defaultBB : endBB);
        builder.CreateCondBr(matched, targetBB, nextBB);
        bodyBBIdx++;
    }

    // Compile the case bodies in order; without an explicit break the next
    // case body falls through (CF script switch fall-through).
    for (size_t cIdx = 0; cIdx < segments.size(); cIdx++) {
        const auto &seg = segments[cIdx];
        if (seg.isDefault) {
            if (!defaultBB) continue;
            builder.SetInsertPoint(defaultBB);
            loopStack.push_back({nullptr, endBB, "", nullptr, true});
            compile_script_expression(seg.bodyTokens, context, module, builder, mainfunc,
                                      out, cgi, server, cookie, application, session, url, form, variables,
                                      cfm_text, cfm_text_size, loopStack);
            loopStack.pop_back();
            if (!builder.GetInsertBlock()->getTerminator()) {
                builder.CreateBr(endBB);
            }
        } else {
            builder.SetInsertPoint(bodyBBs[segToBodyBB[cIdx]]);
            loopStack.push_back({nullptr, endBB, "", nullptr, true});
            compile_script_expression(seg.bodyTokens, context, module, builder, mainfunc,
                                      out, cgi, server, cookie, application, session, url, form, variables,
                                      cfm_text, cfm_text_size, loopStack);
            loopStack.pop_back();
            if (!builder.GetInsertBlock()->getTerminator()) {
                llvm::BasicBlock *fallthrough = endBB;
                for (size_t k = cIdx + 1; k < segments.size(); k++) {
                    if (segments[k].isDefault) { fallthrough = defaultBB ? defaultBB : endBB; break; }
                    fallthrough = bodyBBs[segToBodyBB[k]];
                    break;
                }
                builder.CreateBr(fallthrough);
            }
        }
    }

    builder.SetInsertPoint(endBB);
    return bodyStart + 1;
}

void emit_try_catch_codegen(
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie,
    llvm::Value *application, llvm::Value *session,
    llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const char *cfm_text,
    size_t cfm_text_size,
    std::vector<LoopInfo> &loopStack,
    const std::vector<TryCatchClause> &catches,
    bool hasFinally,
    const std::function<void()> &compileTryBody,
    const std::function<void(size_t catchIndex, llvm::Value *captured)> &compileCatchBody,
    const std::function<void()> &compileFinally)
{
    auto *fCapture = getOrCreateHelper(module, builder, "cf_eh_capture", builder.getPtrTy(),
                                        {builder.getPtrTy()});
    auto *fThrow = getOrCreateHelper(module, builder, "cf_eh_throw", builder.getVoidTy(),
                                      {builder.getPtrTy()});
    auto *fAssign = getOrCreateHelper(module, builder, "cfvariant_assign", builder.getPtrTy(),
                                      {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                       builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                       builder.getPtrTy(), builder.getPtrTy()});
    getOrCreatePersonality(module, builder, mainfunc);

    auto *lpBB = llvm::BasicBlock::Create(context, "try.lp", mainfunc);
    auto *successBB = llvm::BasicBlock::Create(context, "try.success", mainfunc);
    auto *unmatchedBB = llvm::BasicBlock::Create(context, "try.unmatched", mainfunc);
    auto *rethrowBB = llvm::BasicBlock::Create(context, "try.rethrow", mainfunc);
    auto *mergeBB = llvm::BasicBlock::Create(context, "try.merge", mainfunc);
    auto *finallySuccessBB = hasFinally ? llvm::BasicBlock::Create(context, "try.finally.success", mainfunc) : nullptr;
    auto *finallyRethrowBB = hasFinally ? llvm::BasicBlock::Create(context, "try.finally.rethrow", mainfunc) : nullptr;

    std::vector<llvm::BasicBlock*> checkBBs, bodyBBs;
    checkBBs.reserve(catches.size());
    bodyBBs.reserve(catches.size());
    for (size_t c = 0; c < catches.size(); c++) {
        // Dispatch checks are emitted inline in the landing pad block; checkBBs
        // are still allocated so the `winner` comparison branches target them,
        // but each is filled by the preceding comparison's fall-through.
        checkBBs.push_back(llvm::BasicBlock::Create(context, "try.catch.cond", mainfunc));
        bodyBBs.push_back(llvm::BasicBlock::Create(context, "try.catch.body", mainfunc));
    }
    // Holds the best-match clause index (defined in the landing pad, read in
    // each dispatch block).
    llvm::Value *winnerSlot = nullptr;

    auto lpTy = llvm::StructType::get(context, {builder.getPtrTy(), builder.getInt32Ty()});

    // Try/catch-scoped temp cleanup: snapshot the temp registry at try entry
    // and restore it on the success path, in the landing pad (before the catch
    // runs) and after the catch/finally merge. Without this, every expression
    // temp created in the try body (struct/query/exception literals) lives
    // until the enclosing function returns, so a hot try/catch loop grows
    // tl_temp_variants without bound (was BUGS.md "Temp-variant accumulation").
    // The restored temps are cfvariant wrappers; payloads assigned to variables
    // are refcounted/coppied, so nothing referenced stays dangling, and the
    // in-flight C++ exception already copied its message fields.
    auto *fCleanupSave = getOrCreateHelper(module, builder, "cfvariant_cleanup_save", builder.getInt64Ty(), {});
    auto *fCleanupRestore = getOrCreateHelper(module, builder, "cfvariant_cleanup_restore", builder.getVoidTy(),
                                               {builder.getInt64Ty()});
    llvm::Value *trySavepoint = builder.CreateCall(fCleanupSave, {});

    // ---- Try body (calls become invokes unwinding to the landing pad) ----
    EhContext eh{lpBB};
    EhContext *savedEh = g_ehContext;
    g_ehContext = &eh;
    compileTryBody();
    g_ehContext = savedEh;
    if (builder.GetInsertBlock() && !builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(successBB);
    }

    // ---- Success path ----
    builder.SetInsertPoint(successBB);
    emitCall(builder, fCleanupRestore, {trySavepoint});
    if (finallySuccessBB) {
        builder.CreateBr(finallySuccessBB);
    } else {
        builder.CreateBr(mergeBB);
    }

    // ---- Landing pad (catch-all; dispatch done by cf_eh_best_match) ----
    builder.SetInsertPoint(lpBB);
    auto *lp = builder.CreateLandingPad(lpTy, 1, "try.lp.val");
    lp->addClause(llvm::ConstantPointerNull::get(builder.getPtrTy()));
    llvm::Value *exn = builder.CreateExtractValue(lp, 0, "try.exn");
    // Snapshot the live call stack into the in-flight exception now, while the
    // throwing frame is still on the stack, so cfcatch.tagContext reports where
    // the error actually occurred (the frame pops only after capture).
    emitStackCaptureOnException(module, builder, exn);
    emitCall(builder, fCleanupRestore, {trySavepoint});
    if (catches.empty()) {
        builder.CreateBr(unmatchedBB);
    } else {
        // CF matches a catch clause by the CLOSEST exception-class ancestor
        // across ALL clauses (order-independent; ties broken by clause order),
        // so dispatch is a single best-match call instead of a first-match
        // chain.
        llvm::Value *typesArr = createEntryAlloca(builder, mainfunc, builder.getPtrTy(), builder.getInt32(catches.size()));
        for (size_t c = 0; c < catches.size(); c++) {
            auto *p = builder.CreateGEP(builder.getPtrTy(), typesArr, builder.getInt32(c));
            builder.CreateStore(
                builder.CreateGlobalString(llvm::StringRef(catches[c].type.constData(), catches[c].type.length()),
                                           "", 0, module, true),
                p);
        }
        auto *fBestMatch = getOrCreateHelper(module, builder, "cf_eh_best_match", builder.getInt32Ty(),
                                             {builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty()});
        llvm::Value *winner = builder.CreateCall(fBestMatch, {exn, typesArr, builder.getInt32(catches.size())});
        winnerSlot = createEntryAlloca(builder, mainfunc, builder.getInt32Ty());
        builder.CreateStore(winner, winnerSlot);
        builder.CreateBr(checkBBs[0]);
    }

    // ---- Catch-type dispatch (winner index compared per clause) ----
    for (size_t c = 0; c < catches.size(); c++) {
        builder.SetInsertPoint(checkBBs[c]);
        llvm::Value *isC = builder.CreateICmpEQ(builder.CreateLoad(builder.getInt32Ty(), winnerSlot), builder.getInt32(c));
        llvm::BasicBlock *elseBB = (c + 1 < catches.size()) ? checkBBs[c + 1] : unmatchedBB;
        builder.CreateCondBr(isC, bodyBBs[c], elseBB);
    }

    // ---- Catch bodies ----
    for (size_t c = 0; c < catches.size(); c++) {
        builder.SetInsertPoint(bodyBBs[c]);
        llvm::Value *cap = builder.CreateCall(fCapture, {exn});
        if (!catches[c].varName.isEmpty()) {
            emitCall(builder, fAssign, {cgi, server, cookie, application, session, url, form, variables,
                                        builder.CreateGlobalString(
                                            llvm::StringRef(catches[c].varName.constData(), catches[c].varName.length()),
                                            "", 0, module, true),
                                        cap});
        }
        llvm::Value *savedCatchExn = g_currentCatchExn;
        g_currentCatchExn = cap;
        compileCatchBody(c, cap);
        g_currentCatchExn = savedCatchExn;
        if (builder.GetInsertBlock() && !builder.GetInsertBlock()->getTerminator()) {
            if (finallySuccessBB) {
                builder.CreateBr(finallySuccessBB);
            } else {
                builder.CreateBr(mergeBB);
            }
        }
    }

    // ---- Unmatched exception ----
    // Snapshot the exception while it is still in flight (cf_eh_capture does
    // __cxa_begin_catch/__cxa_end_catch), run `finally` if present, then re-raise
    // it with cf_eh_throw. Snapshotting in `unmatchedBB` keeps the value
    // dominant over both the finally-rethrow copy and the direct rethrow.
    builder.SetInsertPoint(unmatchedBB);
    llvm::Value *unmatchedEx = builder.CreateCall(fCapture, {exn});
    if (finallyRethrowBB) {
        builder.CreateBr(finallyRethrowBB);
    } else {
        builder.CreateBr(rethrowBB);
    }

    // ---- Finally (success/catch copy) ----
    if (finallySuccessBB) {
        builder.SetInsertPoint(finallySuccessBB);
        compileFinally();
        if (builder.GetInsertBlock() && !builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(mergeBB);
        }
    }

    // ---- Finally (unmatched copy) then re-raise ----
    if (finallyRethrowBB) {
        builder.SetInsertPoint(finallyRethrowBB);
        compileFinally();
        if (builder.GetInsertBlock() && !builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(rethrowBB);
        }
    }

    // ---- Rethrow: raise a fresh exception carrying the unmatched type/message ----
    builder.SetInsertPoint(rethrowBB);
    emitCall(builder, fThrow, {unmatchedEx});
    builder.CreateUnreachable();

    builder.SetInsertPoint(mergeBB);
    emitCall(builder, fCleanupRestore, {trySavepoint});
}

static size_t compile_script_try_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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
    // ---- Parse the statement shape ----
    size_t i = start + 1;
    while (i < tokens.size() &&
           (tokens[i].token_id == TextParser_cfml_ScriptLineComment ||
            tokens[i].token_id == TextParser_cfml_ScriptBlockComment)) i++;
    if (i >= tokens.size() || tokens[i].token_id != TextParser_cfml_CodeBlock) {
        throw webstrada::exception("Expected '{' after 'try'");
    }
    const TextParserTokenItem &tryBody = tokens[i];
    i++;

    struct CatchClause {
        webstrada::string type;
        webstrada::string varName;
        const TextParserTokenItem *body = nullptr;
    };
    std::vector<CatchClause> catches;
    while (i < tokens.size() &&
           tokens[i].token_id == TextParser_cfml_Keyword &&
           string(cfm_text + tokens[i].position, tokens[i].len).equals("catch")) {
        size_t p = i + 1;
        while (p < tokens.size() &&
               (tokens[p].token_id == TextParser_cfml_ScriptLineComment ||
                tokens[p].token_id == TextParser_cfml_ScriptBlockComment)) p++;
        if (p >= tokens.size() || tokens[p].token_id != TextParser_cfml_Parenthesis) {
            throw webstrada::exception("Expected '(' after 'catch'");
        }
        CatchClause clause;
        const TextParserTokenItem &paramTok = tokens[p];
        // Collect the identifier tokens inside `(type var)`. For a dotted type
        // (`MyApp.SomeError e`) the type text spans from the first to the last
        // identifier.
        std::vector<TextParserTokenItem> idents;
        const std::vector<TextParserTokenItem> *parts = &paramTok.children;
        if (!paramTok.children.empty() &&
            (paramTok.children[0].token_id == TextParser_cfml_ScriptExpression ||
             paramTok.children[0].token_id == TextParser_cfml_Expression)) {
            parts = &paramTok.children[0].children;
        }
        for (const auto &part : *parts) {
            if (part.token_id == TextParser_cfml_Variable) idents.push_back(part);
        }
        if (idents.size() >= 2) {
            clause.varName = string(cfm_text + idents.back().position, idents.back().len).trimmed();
            clause.type = string(cfm_text + idents.front().position,
                                 idents.back().position - idents.front().position).trimmed();
        } else if (idents.size() == 1) {
            clause.type = "any";
            clause.varName = string(cfm_text + idents[0].position, idents[0].len).trimmed();
        } else {
            throw webstrada::exception("Invalid 'catch' clause; expected 'catch (type variable)'");
        }
        size_t cb = p + 1;
        while (cb < tokens.size() &&
               (tokens[cb].token_id == TextParser_cfml_ScriptLineComment ||
                tokens[cb].token_id == TextParser_cfml_ScriptBlockComment)) cb++;
        if (cb >= tokens.size() || tokens[cb].token_id != TextParser_cfml_CodeBlock) {
            throw webstrada::exception("Expected '{' after catch clause");
        }
        clause.body = &tokens[cb];
        catches.push_back(clause);
        i = cb + 1;
    }

    const TextParserTokenItem *finallyBody = nullptr;
    if (i < tokens.size() &&
        tokens[i].token_id == TextParser_cfml_Variable &&
        string(cfm_text + tokens[i].position, tokens[i].len).equals("finally")) {
        size_t fb = i + 1;
        while (fb < tokens.size() &&
               (tokens[fb].token_id == TextParser_cfml_ScriptLineComment ||
                tokens[fb].token_id == TextParser_cfml_ScriptBlockComment)) fb++;
        if (fb >= tokens.size() || tokens[fb].token_id != TextParser_cfml_CodeBlock) {
            throw webstrada::exception("Expected '{' after 'finally'");
        }
        finallyBody = &tokens[fb];
        i = fb + 1;
    }
    const size_t nextIdx = i;

    // CF rejects a script `try { }` with neither catch nor finally at compile
    // time too (verified on the RDS host, CF 2025 — the page 500s):
    // "Context validation error in ''TRY block." (note the '' — the script
    // form names the block with empty quotes, unlike the tag form's CFTRY).
    if (catches.empty() && finallyBody == nullptr) {
        throw webstrada::exception("Context validation error in ''TRY block.");
    }

    // ---- Codegen (shared with the tag form) ----
    std::vector<TryCatchClause> clauses;
    clauses.reserve(catches.size());
    for (const auto &cc : catches) {
        clauses.push_back({cc.type, cc.varName});
    }

    auto compileScriptBody = [&](const TextParserTokenItem *body) {
        if (!body->children.empty() && body->children[0].token_id == TextParser_cfml_ScriptExpression) {
            compile_script_expression(body->children[0].children, context, module, builder, mainfunc,
                                      out, cgi, server, cookie, application, session, url, form, variables,
                                      cfm_text, cfm_text_size, loopStack);
        }
    };

    emit_try_catch_codegen(context, module, builder, mainfunc, out,
                           cgi, server, cookie, application, session, url, form, variables,
                           cfm_text, cfm_text_size, loopStack, clauses, finallyBody != nullptr,
                           [&]() { compileScriptBody(&tryBody); },
                           [&](size_t c, llvm::Value *) { compileScriptBody(catches[c].body); },
                           [&]() { compileScriptBody(finallyBody); });
    return nextIdx;
}

static size_t compile_script_throw_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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
    auto *fCreateStruct = getOrCreateHelper(module, builder, "cfvariant_create_struct", builder.getPtrTy(), {});
    auto *fIndexAssign = getOrCreateHelper(module, builder, "cfvariant_index_assign", builder.getPtrTy(),
                                           {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()});
    auto *fCreateString = getOrCreateHelper(module, builder, "cfvariant_create_string", builder.getPtrTy(), {builder.getPtrTy()});
    auto *fThrow = getOrCreateHelper(module, builder, "cf_eh_throw_new", builder.getVoidTy(),
                                     {builder.getPtrTy(), builder.getInt32Ty()});

    llvm::Value *exStruct = emitCall(builder, fCreateStruct, {});

    auto setKey = [&](const char *key, llvm::Value *val) {
        emitCall(builder, fIndexAssign,
                 {exStruct,
                  emitCall(builder, fCreateString,
                           {builder.CreateGlobalString(key, "", 0, module, true)}),
                  val});
    };

    // Evaluates a slice of expression tokens and returns the resulting value.
    auto evalSlice = [&](const std::vector<TextParserTokenItem> &slice) {
        auto ast = parseTokensToAST(slice, cfm_text);
        return CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie,
                              application, session, url, form, variables, cfm_text);
    };

    size_t i = start + 1;
    while (i < tokens.size() &&
           (tokens[i].token_id == TextParser_cfml_ScriptLineComment ||
            tokens[i].token_id == TextParser_cfml_ScriptBlockComment)) i++;

    if (i < tokens.size() && tokens[i].token_id == TextParser_cfml_Parenthesis) {
        // Function form. The parenthesis holds either named attributes
        // (`name = value, ...`) or a parenthesized message expression.
        const auto &paren = tokens[i];
        const std::vector<TextParserTokenItem> *parts = &paren.children;
        if (!paren.children.empty() &&
            (paren.children[0].token_id == TextParser_cfml_ScriptExpression ||
             paren.children[0].token_id == TextParser_cfml_Expression)) {
            parts = &paren.children[0].children;
        }

        bool hasAssign = false;
        for (const auto &p : *parts) {
            if (isOperatorToken(p.token_id) &&
                string(cfm_text + p.position, p.len).equals("=")) {
                hasAssign = true;
                break;
            }
        }

        if (hasAssign) {
            // Split the top level into `name = value` clauses on Separators.
            std::vector<TextParserTokenItem> clause;
            auto flushClause = [&]() {
                if (clause.empty()) return;
                if (clause.size() < 3 ||
                    clause[0].token_id != TextParser_cfml_Variable ||
                    !isOperatorToken(clause[1].token_id) ||
                    !string(cfm_text + clause[1].position, clause[1].len).equals("=")) {
                    throw webstrada::exception("Invalid throw() attribute; expected 'name = value'");
                }
                webstrada::string attr(cfm_text + clause[0].position, clause[0].len);
                attr.toLower();
                std::vector<TextParserTokenItem> valToks(clause.begin() + 2, clause.end());
                std::vector<TextParserTokenItem> processed = mergeObjectMembers(valToks);
                llvm::Value *val = evalSlice(processed);
                if (attr.equals("type")) setKey("TYPE", val);
                else if (attr.equals("message")) setKey("MESSAGE", val);
                else if (attr.equals("detail")) setKey("DETAIL", val);
                else if (attr.equals("errorcode")) setKey("ERRORCODE", val);
                else if (attr.equals("extendedinfo")) setKey("EXTENDEDINFO", val);
                else throw webstrada::exception("Invalid attribute '" + attr + "' in throw statement");
                clause.clear();
            };
            for (const auto &p : *parts) {
                if (p.token_id == TextParser_cfml_Separator) {
                    flushClause();
                } else {
                    clause.push_back(p);
                }
            }
            flushClause();
        } else {
            // Parenthesized message expression (e.g. `throw (5 + 3)`). A
            // comma-separated argument list maps positionally to CF's
            // throw([message], [type], [detail], [errorcode], [extendedinfo]):
            // each argument is evaluated and stored under its attribute key.
            // Without a comma this is just the message expression.
            std::vector<std::vector<TextParserTokenItem>> args;
            std::vector<TextParserTokenItem> arg;
            bool hasComma = false;
            for (const auto &p : *parts) {
                if (p.token_id == TextParser_cfml_Separator) {
                    hasComma = true;
                    args.push_back(arg);
                    arg.clear();
                } else {
                    arg.push_back(p);
                }
            }
            if (!arg.empty() || hasComma) {
                args.push_back(arg);
            }
            if (hasComma) {
                const char *keys[] = {"MESSAGE", "TYPE", "DETAIL", "ERRORCODE", "EXTENDEDINFO"};
                size_t maxIdx = sizeof(keys) / sizeof(keys[0]);
                if (args.size() > maxIdx) {
                    throw webstrada::exception("Too many arguments in throw statement");
                }
                for (size_t ai = 0; ai < args.size(); ai++) {
                    std::vector<TextParserTokenItem> processed = mergeObjectMembers(args[ai]);
                    setKey(keys[ai], evalSlice(processed));
                }
            } else if (!parts->empty()) {
                std::vector<TextParserTokenItem> processed = mergeObjectMembers(*parts);
                setKey("MESSAGE", evalSlice(processed));
            }
        }
        i++;
    } else {
        // Bare form: optional message expression up to the ExpressionEnd.
        std::vector<TextParserTokenItem> msgToks;
        while (i < tokens.size() && tokens[i].token_id != TextParser_cfml_ExpressionEnd) {
            if (tokens[i].token_id != TextParser_cfml_ScriptLineComment &&
                tokens[i].token_id != TextParser_cfml_ScriptBlockComment) {
                msgToks.push_back(tokens[i]);
            }
            i++;
        }
        if (!msgToks.empty()) {
            std::vector<TextParserTokenItem> processed = mergeObjectMembers(msgToks);
            setKey("MESSAGE", evalSlice(processed));
        }
    }

    while (i < tokens.size() &&
           (tokens[i].token_id == TextParser_cfml_ScriptLineComment ||
            tokens[i].token_id == TextParser_cfml_ScriptBlockComment)) i++;
    if (i < tokens.size() && tokens[i].token_id == TextParser_cfml_ExpressionEnd) i++;

    emitCall(builder, fThrow, {exStruct, builder.getInt32(1)});
    builder.CreateUnreachable();
    auto deadBB = llvm::BasicBlock::Create(context, "throw.cont", mainfunc);
    builder.SetInsertPoint(deadBB);
    return i;
}

static size_t compile_script_statement(
    const std::vector<TextParserTokenItem> &tokens,
    size_t start,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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
    std::vector<LoopInfo> &loopStack
) {
    if (start >= tokens.size()) return start;

    const auto &token = tokens[start];

    // Keep the top call-stack frame's line in sync as this statement executes
    // (comments/separators are skipped by the caller, so token[start] is a real
    // statement). Recursively compiled bodies (if/for/while/switch/try) update
    // the same frame with their own statements' lines.
    if (token.token_id != TextParser_cfml_ScriptLineComment &&
        token.token_id != TextParser_cfml_ScriptBlockComment) {
        emitStackSetLine(module, builder, lineOfOffset(cfm_text, token.position));
    }

    if (token.token_id == TextParser_cfml_Keyword &&
        string(cfm_text + token.position, token.len).equals("if") &&
        start + 1 < tokens.size() &&
        tokens[start + 1].token_id == TextParser_cfml_Parenthesis) {
        return compile_script_if_statement(tokens, start, context, module, builder, mainfunc,
                                           out, cgi, server, cookie, application, session, url, form, variables,
                                           cfm_text, cfm_text_size, loopStack);
    }

    // `try { } catch (type var) { } finally { }`.
    if (token.token_id == TextParser_cfml_Keyword &&
        string(cfm_text + token.position, token.len).equals("try")) {
        size_t after = start + 1;
        while (after < tokens.size() &&
               (tokens[after].token_id == TextParser_cfml_ScriptLineComment ||
                tokens[after].token_id == TextParser_cfml_ScriptBlockComment)) after++;
        if (after < tokens.size() && tokens[after].token_id == TextParser_cfml_CodeBlock) {
            return compile_script_try_statement(tokens, start, context, module, builder, mainfunc,
                                                out, cgi, server, cookie, application, session, url, form, variables,
                                                cfm_text, cfm_text_size, loopStack);
        }
    }

    // Named function declaration: `function name(...) { ... }`. Registration is
    // emitted elsewhere (page-level UDFs at template start, nested ones at the
    // enclosing function's body entry), so the statement itself emits nothing.
    if (token.token_id == TextParser_cfml_Keyword &&
        string(cfm_text + token.position, token.len).equals("function") &&
        start + 1 < tokens.size() &&
        tokens[start + 1].token_id == TextParser_cfml_Function) {
        UdfDef ignored;
        return parseFunctionDecl(tokens, start, cfm_text, ignored);
    }

    // `return [expr];` inside a function body: store the (coerced) value in the
    // return slot and jump to the common exit block. A bare `return;` returns
    // undefined.
    if (token.token_id == TextParser_cfml_Variable &&
        string(cfm_text + token.position, token.len).equals("return")) {
        if (!g_returnCtx) {
            throw webstrada::exception("'return' is only valid inside a function");
        }
        size_t after = start + 1;
        while (after < tokens.size() &&
               (tokens[after].token_id == TextParser_cfml_ScriptLineComment ||
                tokens[after].token_id == TextParser_cfml_ScriptBlockComment)) {
            after++;
        }
        if (after < tokens.size() && tokens[after].token_id == TextParser_cfml_ExpressionEnd) {
            builder.CreateBr(g_returnCtx->exitBB);
            auto deadBB = llvm::BasicBlock::Create(context, "return.cont", mainfunc);
            builder.SetInsertPoint(deadBB);
            return after + 1;
        }
        std::vector<TextParserTokenItem> exprToks;
        while (after < tokens.size() && tokens[after].token_id != TextParser_cfml_ExpressionEnd) {
            if (tokens[after].token_id != TextParser_cfml_ScriptLineComment &&
                tokens[after].token_id != TextParser_cfml_ScriptBlockComment) {
                exprToks.push_back(tokens[after]);
            }
            after++;
        }
        if (!exprToks.empty()) {
            auto ast = parseTokensToAST(exprToks, cfm_text);
            llvm::Value *val = CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fCoerce = module->getFunction("cf_udf_coerce_return");
            if (!fCoerce) fCoerce = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cf_udf_coerce_return", module);
            llvm::Value *coerced = emitCall(builder, fCoerce, {val,
                builder.CreateGlobalString(llvm::StringRef(g_returnCtx->returnType), "", 0, module, true),
                builder.CreateGlobalString(llvm::StringRef(g_returnCtx->funcName), "", 0, module, true)});
            builder.CreateStore(coerced, g_returnCtx->retSlot);
        }
        builder.CreateBr(g_returnCtx->exitBB);
        auto deadBB = llvm::BasicBlock::Create(context, "return.cont", mainfunc);
        builder.SetInsertPoint(deadBB);
        return (after < tokens.size() && tokens[after].token_id == TextParser_cfml_ExpressionEnd) ? after + 1 : after;
    }

    // `elseif` is not a ColdFusion script keyword — only `else if` (two
    // keywords) is valid; CF 2021 rejects a template that uses it. The
    // textparser tokenizes it as a Function (it is absent from the Keyword
    // regex), so without this guard `if (c) A; elseif (c2) B;` would fall into
    // the generic statement path where the leading `elseif (c2)` call is
    // dropped and `B;` would run silently.
    if (token.token_id == TextParser_cfml_Function) {
        webstrada::string fname(cfm_text + token.position, token.len);
        fname = fname.trimmed();
        int paren = fname.indexOf('(');
        if (paren >= 0) fname = fname.mid(0, paren);
        fname = fname.trimmed();
        fname.toLower();
        if (fname.equals("elseif")) {
            throw webstrada::exception("Unexpected 'elseif' in cfscript (use 'else if')");
        }
        // Loop/switch statement keywords are tokenized as Function tokens
        // (the grammar's Function regex matches `name(`); dispatch them to
        // their dedicated compilers when followed by a Parenthesis.
        if (start + 1 < tokens.size() && tokens[start + 1].token_id == TextParser_cfml_Parenthesis) {
            if (fname.equals("throw")) {
                return compile_script_throw_statement(tokens, start, context, module, builder, mainfunc,
                                                      out, cgi, server, cookie, application, session, url, form, variables,
                                                      cfm_text, cfm_text_size, loopStack);
            }
            if (fname.equals("for")) {
                return compile_script_for(tokens, start, context, module, builder, mainfunc,
                                          out, cgi, server, cookie, application, session, url, form, variables,
                                          cfm_text, cfm_text_size, loopStack);
            }
            if (fname.equals("while")) {
                return compile_script_while(tokens, start, context, module, builder, mainfunc,
                                            out, cgi, server, cookie, application, session, url, form, variables,
                                            cfm_text, cfm_text_size, loopStack);
            }
            if (fname.equals("switch")) {
                return compile_script_switch(tokens, start, context, module, builder, mainfunc,
                                             out, cgi, server, cookie, application, session, url, form, variables,
                                             cfm_text, cfm_text_size, loopStack);
            }
        }
    }

    // `do { ... } while (cond);` — the textparser tokenizes `do` as a Variable.
    if (token.token_id == TextParser_cfml_Variable &&
        string(cfm_text + token.position, token.len).equals("do") &&
        start + 1 < tokens.size() &&
        tokens[start + 1].token_id == TextParser_cfml_CodeBlock) {
        return compile_script_do_while(tokens, start, context, module, builder, mainfunc,
                                       out, cgi, server, cookie, application, session, url, form, variables,
                                       cfm_text, cfm_text_size, loopStack);
    }

    // `break;` and `continue;` inside loops/switch.
    if (token.token_id == TextParser_cfml_Variable &&
        start + 1 < tokens.size() &&
        tokens[start + 1].token_id == TextParser_cfml_ExpressionEnd) {
        webstrada::string kw(cfm_text + token.position, token.len);
        kw.toLower();
        if (kw.equals("break")) {
            if (loopStack.empty()) throw webstrada::exception("'break' is only valid inside a loop or switch");
            builder.CreateBr(loopStack.back().endBB);
            auto deadBB = llvm::BasicBlock::Create(context, "break.cont", mainfunc);
            builder.SetInsertPoint(deadBB);
            return start + 2;
        }
        if (kw.equals("continue")) {
            for (auto it = loopStack.rbegin(); it != loopStack.rend(); ++it) {
                if (!it->isSwitch) {
                    builder.CreateBr(it->incBB);
                    auto deadBB = llvm::BasicBlock::Create(context, "continue.cont", mainfunc);
                    builder.SetInsertPoint(deadBB);
                    return start + 2;
                }
            }
            throw webstrada::exception("'continue' is only valid inside a loop");
        }
    }

    // `abort;` — stops template processing with an uncatchable abort (like
    // <cfabort>). Previously a bare `abort;` fell through to the expression
    // path and resolved as an undefined variable, so a `try { abort; }` caught
    // it; CF aborts the request (uncatchable).
    if (token.token_id == TextParser_cfml_Variable &&
        string(cfm_text + token.position, token.len).equals("abort") &&
        start + 1 < tokens.size() &&
        tokens[start + 1].token_id == TextParser_cfml_ExpressionEnd) {
        auto *cfabort = module->getFunction("cfabort");
        if (!cfabort) cfabort = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), false), llvm::Function::InternalLinkage, "cfabort", module);
        emitCall(builder, cfabort, {});
        builder.CreateUnreachable();
        auto deadBB = llvm::BasicBlock::Create(context, "abort.cont", mainfunc);
        builder.SetInsertPoint(deadBB);
        return start + 2;
    }

    // `exit;` — the script form of <cfexit>: inside a function body it returns
    // undefined (like a bare `return;`); outside it aborts the current
    // template page with the same uncatchable exit_exception as <cfexit>
    // (swallowed at include/construction/prelude boundaries).
    if (token.token_id == TextParser_cfml_Variable &&
        string(cfm_text + token.position, token.len).equals("exit") &&
        start + 1 < tokens.size() &&
        tokens[start + 1].token_id == TextParser_cfml_ExpressionEnd) {
        if (g_returnCtx) {
            builder.CreateBr(g_returnCtx->exitBB);
        } else {
            auto *cfexit = module->getFunction("cf_exit");
            if (!cfexit) cfexit = llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), false), llvm::Function::InternalLinkage, "cf_exit", module);
            emitCall(builder, cfexit, {});
            builder.CreateUnreachable();
        }
        auto deadBB = llvm::BasicBlock::Create(context, "exit.cont", mainfunc);
        builder.SetInsertPoint(deadBB);
        return start + 2;
    }

    // `rethrow;` — re-raises the exception caught by the innermost enclosing
    // catch block (an error anywhere else, matching CF's compile-time
    // rejection of `rethrow;` outside a catch).
    if (token.token_id == TextParser_cfml_Variable &&
        string(cfm_text + token.position, token.len).equals("rethrow") &&
        start + 1 < tokens.size() &&
        tokens[start + 1].token_id == TextParser_cfml_ExpressionEnd) {
        if (!g_currentCatchExn) {
            throw webstrada::exception("'rethrow' is only valid inside a catch block");
        }
        auto *fThrow = getOrCreateHelper(module, builder, "cf_eh_throw", builder.getVoidTy(),
                                         {builder.getPtrTy()});
        emitCall(builder, fThrow, {g_currentCatchExn});
        builder.CreateUnreachable();
        auto deadBB = llvm::BasicBlock::Create(context, "rethrow.cont", mainfunc);
        builder.SetInsertPoint(deadBB);
        return start + 2;
    }

    // Bare `throw;` / `throw <message>;` (the parenthesized `throw(...)` form
    // is dispatched from the Function branch above).
    if (token.token_id == TextParser_cfml_Variable &&
        string(cfm_text + token.position, token.len).equals("throw")) {
        return compile_script_throw_statement(tokens, start, context, module, builder, mainfunc,
                                              out, cgi, server, cookie, application, session, url, form, variables,
                                              cfm_text, cfm_text_size, loopStack);
    }

    std::vector<TextParserTokenItem> stmtTokens;
    size_t i = start;
    while (i < tokens.size() && tokens[i].token_id != TextParser_cfml_ExpressionEnd) {
        const auto &t = tokens[i];
        bool varIsNamedArg = (t.token_id == TextParser_cfml_Keyword &&
                              string(cfm_text + t.position, t.len).equals("var") &&
                              i + 1 < tokens.size() &&
                              isOperatorToken(tokens[i + 1].token_id));
        if (varIsNamedArg) {
            std::string opText = tokenText(tokens[i + 1], cfm_text);
            while (!opText.empty() && isspace(opText.front())) opText.erase(opText.begin());
            while (!opText.empty() && isspace(opText.back())) opText.pop_back();
            varIsNamedArg = (opText == "=");
        }
        if (t.token_id == TextParser_cfml_Keyword && string(cfm_text + t.position, t.len).equals("var")) {
            // `var` used as a named-argument name (`two(var="x")`) is a
            // variable reference, not a local declaration.
            if (varIsNamedArg) {
                // fall through: keep the token in stmtTokens
            } else if (!g_compileInFunctionBody) {
                std::string vname = "";
                if (i + 1 < tokens.size() && tokens[i + 1].token_id == TextParser_cfml_Variable) {
                    vname = tokenText(tokens[i + 1], cfm_text);
                }
                throw webstrada::exception(webstrada::string(("The local variable " + vname + " cannot be declared outside of a function.").c_str()));
            }
        }
        if (t.token_id != TextParser_cfml_ScriptLineComment &&
            t.token_id != TextParser_cfml_ScriptBlockComment &&
            !(t.token_id == TextParser_cfml_Keyword && string(cfm_text + t.position, t.len).equals("var") && !varIsNamedArg)) {
            stmtTokens.push_back(t);
        }
        i++;
    }

    if (i < tokens.size() && tokens[i].token_id == TextParser_cfml_ExpressionEnd) {
        i++;
    }

    if (!stmtTokens.empty()) {
        auto processed = mergeObjectMembers(stmtTokens);
        auto ast = parseTokensToAST(processed, cfm_text);
        CompileExprAST(module, builder, mainfunc, ast, cgi, server, cookie, application, session, url, form, variables, cfm_text);
    }

    return i;
}

void compile_script_expression(
    const std::vector<TextParserTokenItem> &tokens,
    llvm::LLVMContext &context,
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *mainfunc,
    llvm::Value *out,
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
    llvm::Value *savedOut = g_currentOut;
    g_currentOut = out;

    size_t i = 0;
    while (i < tokens.size()) {
        const auto &token = tokens[i];

        if (token.token_id == TextParser_cfml_ScriptLineComment ||
            token.token_id == TextParser_cfml_ScriptBlockComment ||
            token.token_id == TextParser_cfml_Separator) {
            i++;
            continue;
        }

        i = compile_script_statement(tokens, i, context, module, builder, mainfunc,
                                     out, cgi, server, cookie, application, session, url, form, variables,
                                     cfm_text, cfm_text_size, loopStack);
    }

    g_currentOut = savedOut;
}

} // namespace webstrada
