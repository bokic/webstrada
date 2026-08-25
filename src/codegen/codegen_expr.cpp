/**
 * @file codegen_expr.cpp
 * Code generation: expr.
 */

#include "codegen_internal.h"

#include <webstrada/cf8.h>
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

// ---- definitions ----

static std::unique_ptr<ExprAST> cloneExpr(const std::unique_ptr<ExprAST> &node)
{
    auto copy = std::make_unique<ExprAST>();
    copy->type = node->type;
    copy->int_val = node->int_val;
    copy->long_val = node->long_val;
    copy->float_val = node->float_val;
    copy->bool_val = node->bool_val;
    copy->string_val = node->string_val;
    copy->op_val = node->op_val;
    copy->float_literal_text = node->float_literal_text;
    copy->token = node->token;
    copy->fromParen = node->fromParen;
    copy->isPre = node->isPre;
    copy->isChainBase = node->isChainBase;
    copy->closureParams = node->closureParams;
    copy->closureParamTypes = node->closureParamTypes;
    copy->closureParamDefaults = node->closureParamDefaults;
    copy->closureBody = node->closureBody;
    if (node->left) copy->left = cloneExpr(node->left);
    if (node->right) copy->right = cloneExpr(node->right);
    for (const auto &a : node->args) copy->args.push_back(cloneExpr(a));
    for (const auto &e : node->elements) copy->elements.push_back(cloneExpr(e));
    copy->structLiteralKeys = node->structLiteralKeys;
    for (const auto &v : node->structLiteralValues) copy->structLiteralValues.push_back(cloneExpr(v));
    return copy;
}

static int getOpPrecedence(const std::string &op, bool unary) {
    if (unary) {
        // Unary -/+ bind tighter than '^' (CF: -2^2 == (-2)^2 == 4) but
        // looser than member access (CF: -x.y == -(x.y)).
        if (op == "-" || op == "+") return 13;
        if (op == "NOT" || op == "!") return 6;
        return 0;
    }

    if (op == "=" || op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=" || op == "&=") return 0;
    if (op == "IMP") return 1;
    if (op == "EQV") return 2;
    if (op == "XOR") return 3;
    if (op == "OR" || op == "||") return 4;
    if (op == "AND" || op == "&&") return 5;
    if (op == "EQ" || op == "NEQ" || op == "LT" || op == "LTE" || op == "LE" || op == "GT" || op == "GTE" || op == "GE" ||
        op == "IS" || op == "EQUAL" || op == "GREATER THAN" || op == "LESS THAN" ||
        op == "CONTAINS" || op == "DOES NOT CONTAIN" || op == "IS NOT" || op == "==" || op == "!=" ||
        op == ">=" || op == "<=" || op == ">" || op == "<") return 8;
    if (op == "&") return 9;
    if (op == "+" || op == "-") return 10;
    if (op == "*" || op == "/" || op == "\\" || op == "MOD" || op == "%") return 11;
    if (op == "^") return 12;
    if (op == ".") return 14;
    return 0;
}

static bool isRightAssociative(const std::string &op) {
    // CF's '^' is left-associative (2^3^2 == (2^3)^2 == 64, verified on CF 2021).
    return op == "=" || op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%=" || op == "&=";
}

static bool isSimpleNameChain(const ExprAST *n) {
    if (!n) return false;
    if (n->type == ExprAST::Variable) return true;
    if (n->type == ExprAST::BinaryOp && n->op_val == ".") {
        return isSimpleNameChain(n->left.get()) && n->right && n->right->type == ExprAST::Variable;
    }
    return false;
}

// mergeObjectMembers glues `Variable(k2) ObjectMember(.) Variable(k3)` into one
// Variable("k2.k3") token even mid-chain (after an ArrayIndex hop). A Variable
// name containing '.' can therefore only come from such merging — CFML
// identifiers cannot contain dots — and must be walked as separate member hops,
// never used as a single literal struct key.
static std::vector<std::string> splitMemberPath(const std::string &name)
{
    std::vector<std::string> parts;
    std::string cur;
    for (char c : name) {
        if (c == '.') {
            while (!cur.empty() && isspace(static_cast<unsigned char>(cur.back()))) cur.pop_back();
            size_t s = 0;
            while (s < cur.size() && isspace(static_cast<unsigned char>(cur[s]))) s++;
            parts.push_back(cur.substr(s));
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    while (!cur.empty() && isspace(static_cast<unsigned char>(cur.back()))) cur.pop_back();
    size_t s = 0;
    while (s < cur.size() && isspace(static_cast<unsigned char>(cur[s]))) s++;
    parts.push_back(cur.substr(s));
    return parts;
}

// Emits chained cfvariant_index calls for every member-name segment in `parts`
// starting from `lhs` and returns the final slot pointer.
static llvm::Value *emitMemberWalk(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *lhs,
                                   const std::vector<std::string> &parts)
{
    auto *fIdx = module->getFunction("cfvariant_index");
    if (!fIdx) fIdx = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index", module);
    for (const auto &seg : parts) {
        auto *fStr = module->getFunction("cfvariant_create_string");
        if (!fStr) fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
        auto *keyVal = emitCall(builder, fStr, {builder.CreateGlobalString(seg, "", 0, module, true)});
        lhs = emitCall(builder, fIdx, {lhs, keyVal});
    }
    return lhs;
}

static bool isCfParamTypeName(const std::string &s)
{
    std::string t;
    for (char c : s) t.push_back((char)tolower((unsigned char)c));
    static const std::set<std::string> types = {
        "any", "array", "binary", "boolean", "component", "date", "float", "guid",
        "image", "int", "integer", "long", "numeric", "query", "string", "struct",
        "uuid", "variablename", "xml"
    };
    return types.count(t) != 0;
}

void parseParamList(const TextParserTokenItem &parenToken, const char *cfm_text,
                           std::vector<std::string> &names,
                           std::vector<std::string> &types,
                           std::vector<std::vector<TextParserTokenItem>> &defaults)
{
    const TextParserTokenItem *expr = nullptr;
    if (!parenToken.children.empty() &&
        (parenToken.children[0].token_id == TextParser_cfml_Expression ||
         parenToken.children[0].token_id == TextParser_cfml_ScriptExpression)) {
        expr = &parenToken.children[0];
    }
    if (!expr) return;

    std::vector<TextParserTokenItem> group;
    auto flush = [&]() {
        if (group.empty()) return;
        std::string name, type;
        std::vector<TextParserTokenItem> defToks;
        size_t start = 0;
        if (group.size() >= 2 && group[0].token_id == TextParser_cfml_Variable &&
            group[1].token_id == TextParser_cfml_Variable) {
            type = std::string(cfm_text + group[0].position, group[0].len);
            name = std::string(cfm_text + group[1].position, group[1].len);
            start = 2;
        } else {
            name = std::string(cfm_text + group[0].position, group[0].len);
            start = 1;
        }
        if (start < group.size() && isOperatorToken(group[start].token_id)) {
            std::string op(cfm_text + group[start].position, group[start].len);
            while (!op.empty() && isspace(op.front())) op.erase(op.begin());
            while (!op.empty() && isspace(op.back())) op.pop_back();
            if (op == "=") {
                defToks.assign(group.begin() + start + 1, group.end());
            }
        }
        names.push_back(name);
        types.push_back(type);
        defaults.push_back(std::move(defToks));
        group.clear();
    };
    for (const auto &t : expr->children) {
        if (t.token_id == TextParser_cfml_Separator) flush();
        else if (t.token_id == TextParser_cfml_ScriptLineComment ||
                 t.token_id == TextParser_cfml_ScriptBlockComment) continue;
        else group.push_back(t);
    }
    flush();
}

std::unique_ptr<ExprAST> parseTokensToAST(const std::vector<TextParserTokenItem> &tokens, const char *cfm_text, bool sharpContext) {
    std::vector<std::unique_ptr<ExprAST>> operandStack;
    std::vector<std::pair<std::string, bool>> opStack;

    auto applyOp = [&]() {
        if (opStack.empty()) return;
        auto opInfo = opStack.back();
        opStack.pop_back();

        auto node = std::make_unique<ExprAST>();
        if (opInfo.second) { // unary
            if (operandStack.empty()) throw webstrada::exception("Unary operator missing operand");
            node->type = ExprAST::UnaryOp;
            node->op_val = opInfo.first;
            node->right = std::move(operandStack.back());
            operandStack.pop_back();
        } else { // binary
            if (operandStack.size() < 2)
            {
                std::string ctx;
                for (const auto &t : tokens)
                {
                    if (!ctx.empty()) ctx += ' ';
                    ctx += std::string(cfm_text + t.position, std::min(t.len, (size_t)40));
                }
                if (ctx.size() > 200) ctx = ctx.substr(0, 200) + "...";
                throw webstrada::exception(("Binary operator missing operand in expression: " + ctx).c_str());
            }
            node->right = std::move(operandStack.back());
            operandStack.pop_back();
            node->left = std::move(operandStack.back());
            operandStack.pop_back();

            // CF rejects a member access directly on a parenthesized
            // expression ((s).a), though it allows one on a function call or a
            // bare variable/struct literal.
            if (opInfo.first == "." && node->left->fromParen) {
                throw webstrada::exception("Cannot access a member of a parenthesized expression");
            }

            // The left operand of member access is a chain base: an undefined
            // variable here must throw, never fall back to a built-in method
            // handle (CF: pi.foo -> "Variable PI is undefined.").
            if (opInfo.first == "." && node->left->type == ExprAST::Variable) {
                node->left->isChainBase = true;
            }
            // A '.' whose left is itself a '.' BinaryOp (a.b.c) marks that inner
            // op as chained: the chain-base ELEMENT error only applies to a
            // terminal single-dot access (CF: a.b -> "Element B is undefined in
            // A.", a.b.c -> "Variable A is undefined.").
            if (opInfo.first == "." && node->left->type == ExprAST::BinaryOp &&
                node->left->op_val == ".") {
                node->left->isChainedMember = true;
            }

            if (opInfo.first == "=" || opInfo.first == "+=" || opInfo.first == "-=" || opInfo.first == "*=" || opInfo.first == "/=" || opInfo.first == "%=" || opInfo.first == "&=") {
                node->type = ExprAST::Assign;
                node->op_val = opInfo.first;
            } else {
                node->type = ExprAST::BinaryOp;
                node->op_val = opInfo.first;
            }
        }
        operandStack.push_back(std::move(node));
    };

    // Turns a parsed `fn(args)` node into a MemberCall when the base is a
    // member-access context: mergeObjectMembers has left the top operand as a
    // Variable ending with '.', or a '.' operator is pending on the stack
    // (inside a function's argument list, where merging is not applied).
    // Returns true when the call was turned into a MemberCall.
    bool nextCanBeUnary = true;
    auto tryMemberCall = [&](std::unique_ptr<ExprAST> &callNode) -> bool {
        if (!operandStack.empty() && operandStack.back()->type == ExprAST::Variable) {
            std::string &base = operandStack.back()->string_val;
            if (!base.empty() && base.back() == '.') {
                base.pop_back();
                auto mc = std::make_unique<ExprAST>();
                mc->type = ExprAST::MemberCall;
                mc->left = std::move(operandStack.back());
                mc->right = std::move(callNode);
                operandStack.pop_back();
                operandStack.push_back(std::move(mc));
                nextCanBeUnary = false;
                return true;
            }
        }
        if (!opStack.empty() && opStack.back().first == "." && !opStack.back().second &&
            !operandStack.empty()) {
            opStack.pop_back();
            auto mc = std::make_unique<ExprAST>();
            mc->type = ExprAST::MemberCall;
            mc->left = std::move(operandStack.back());
            mc->right = std::move(callNode);
            operandStack.pop_back();
            operandStack.push_back(std::move(mc));
            nextCanBeUnary = false;
            return true;
        }
        return false;
    };

    for (size_t i = 0; i < tokens.size(); i++) {
        const auto &tok = tokens[i];

        if (tok.token_id == TextParser_cfml_Number) {
            if (!nextCanBeUnary) throw webstrada::exception("Unexpected tokens in expression");
            std::string text(cfm_text + tok.position, tok.len);
            auto node = std::make_unique<ExprAST>();
            if (text.find('.') != std::string::npos || text.find('e') != std::string::npos || text.find('E') != std::string::npos) {
                node->type = ExprAST::LiteralFloat;
                node->float_val = std::stod(text);
                // CF preserves the literal text of unsigned float literals
                // (8.0 → "8.0", 8.10 → "8.10", 5.0E2 → "5.0E2"). '-8.0'
                // parses as unary minus → computed ("-8"), a leading '+'
                // is stripped (+8.0 → "8.0").
                if (!text.empty() && text[0] != '-') {
                    node->float_literal_text = (text[0] == '+') ? text.substr(1) : text;
                }
            } else {
                // CF parses integer literals as Integer -> Long -> CFBigInteger,
                // all of which render as their digits. Values that fit in a
                // signed 32-bit int stay int; larger values within int64 become
                // Long (2147483648, 9007199254740992, 9223372036854775807 render
                // as written); '-'-prefixed literals (the tokenizer emits them as
                // a single token, e.g. '-2147483649') render through the float
                // formatter, matching CF ('-2147483649' -> digits,
                // '-9223372036854775808' -> '-9.22337203685E+018'); values beyond
                // int64 fall back to Float carrying their literal text
                // (9223372036854775808 renders as written).
                bool fitsInt32 = true;
                long long ll = 0;
                try {
                    ll = std::stoll(text);
                } catch (...) {
                    fitsInt32 = false; // beyond INT64: promoted to float below
                }
                if (fitsInt32 && ll >= -2147483648LL && ll <= 2147483647LL) {
                    node->type = ExprAST::LiteralInt;
                    node->int_val = static_cast<int>(ll);
                } else if (fitsInt32 && text[0] != '-') {
                    node->type = ExprAST::LiteralLong;
                    node->long_val = ll;
                } else {
                    node->type = ExprAST::LiteralFloat;
                    node->float_val = std::stod(text);
                    if (!text.empty() && text[0] != '-') {
                        node->float_literal_text = (text[0] == '+') ? text.substr(1) : text;
                    }
                }
            }
            operandStack.push_back(std::move(node));
            nextCanBeUnary = false;
        }
        else if (tok.token_id == TextParser_cfml_Boolean) {
            if (!nextCanBeUnary) throw webstrada::exception("Unexpected tokens in expression");
            std::string text(cfm_text + tok.position, tok.len);
            for (auto &c : text) c = tolower(c);
            auto node = std::make_unique<ExprAST>();
            node->type = ExprAST::LiteralBool;
            node->bool_val = (text == "true" || text == "yes");
            operandStack.push_back(std::move(node));
            nextCanBeUnary = false;
        }
        else if (tok.token_id == TextParser_cfml_SingleString || tok.token_id == TextParser_cfml_DoubleString) {
            if (!nextCanBeUnary) throw webstrada::exception("Unexpected tokens in expression");
            auto node = std::make_unique<ExprAST>();
            node->type = ExprAST::LiteralString;
            node->token = tok;
            operandStack.push_back(std::move(node));
            nextCanBeUnary = false;
        }
        else if (tok.token_id == TextParser_cfml_Variable) {
            std::string name(cfm_text + tok.position, tok.len);
            std::string uname = name;
            while(!uname.empty() && isspace(uname.front())) uname.erase(uname.begin());
            while(!uname.empty() && isspace(uname.back())) uname.pop_back();
            for (auto &c : uname) c = toupper(c);

            auto isOperatorWord = [](const std::string &s) {
                return s == "NOT" || s == "AND" || s == "OR" || s == "XOR" || s == "EQV" || s == "IMP" || s == "MOD" ||
                       s == "EQ" || s == "NEQ" || s == "LT" || s == "LTE" || s == "LE" || s == "GT" || s == "GTE" || s == "GE" ||
                       s == "IS" || s == "EQUAL" || s == "CONTAINS" || s == "DOES NOT CONTAIN" || s == "IS NOT";
            };

            // An operator keyword used as a struct property (`cacheresult.contains`,
            // `s.eq`) is not an operator: when the previous token is the '.'
            // ObjectMember (unmerged member access, e.g. tag-attribute
            // expressions) the word is a member name, never the CFML
            // CONTAINS/EQ/... operator (same rule the operator-token branch
            // applies to member names).
            bool operatorWordAsMember = isOperatorWord(uname) &&
                i > 0 && tokens[i - 1].token_id == TextParser_cfml_ObjectMember;

            if (isOperatorWord(uname) && !operatorWordAsMember) {
                bool isUnary = nextCanBeUnary;
                int prec = getOpPrecedence(uname, isUnary);

                while (!opStack.empty()) {
                    auto topOp = opStack.back();
                    int topPrec = getOpPrecedence(topOp.first, topOp.second);

                    if (topPrec > prec || (topPrec == prec && !isRightAssociative(uname))) {
                        applyOp();
                    } else {
                        break;
                    }
                }

                opStack.push_back({uname, isUnary});
                nextCanBeUnary = true;
            } else if (uname == "NEW" || (uname.rfind("NEW ", 0) == 0 && uname.back() == '.')) {
                // cfscript `new path.Component(args)`: mergeObjectMembers has
                // already merged a dotted path into a Variable like
                // "new components ." (or just "new" for a plain `new c1()`);
                // the final segment is the following Function token. Build a
                // NewExpr carrying the component dot-path and its arguments.
                if (!nextCanBeUnary) throw webstrada::exception("Unexpected tokens in expression");
                std::string path;
                if (uname == "NEW") {
                    path = "";
                } else {
                    // Use the raw (original-case) text; the merged Variable is
                    // "new components ." (uname is uppercased).
                    path = name;
                    size_t sp = 0;
                    while (sp < path.size() && isspace((unsigned char)path[sp])) sp++;
                    size_t nPos = path.find_first_of(" \t\r\n", sp);
                    if (nPos != std::string::npos && nPos + 1 < path.size()) {
                        path = path.substr(nPos + 1);
                    } else {
                        path = "";
                    }
                    while (!path.empty() && (path.back() == '.' || isspace((unsigned char)path.back()))) path.pop_back();
                    while (!path.empty() && isspace((unsigned char)path.front())) path.erase(path.begin());
                }
                size_t ni = i + 1;
                if (ni >= tokens.size() || tokens[ni].token_id != TextParser_cfml_Function) {
                    throw webstrada::exception("new requires a component path");
                }
                std::string fname(cfm_text + tokens[ni].position, tokens[ni].len);
                size_t paren = fname.find('(');
                if (paren != std::string::npos) fname = fname.substr(0, paren);
                while(!fname.empty() && isspace(fname.back())) fname.pop_back();
                if (!path.empty()) path += ".";
                path += fname;

                auto node = std::make_unique<ExprAST>();
                node->type = ExprAST::NewExpr;
                node->op_val = path;
                i = ni;  // consume the Function token

                // Parse the argument list from the following Parenthesis token
                // (mirrors the FuncCall argument parsing below).
                if (i + 1 < tokens.size() && tokens[i + 1].token_id == TextParser_cfml_Parenthesis) {
                    const auto &subExprTok = tokens[i + 1];
                    i++;
                    if (!subExprTok.children.empty() &&
                        (subExprTok.children[0].token_id == TextParser_cfml_Expression ||
                         subExprTok.children[0].token_id == TextParser_cfml_ScriptExpression)) {
                        const auto &exprTok = subExprTok.children[0];
                        std::vector<TextParserTokenItem> argToks;
                        for (const auto &child : exprTok.children) {
                            if (child.token_id == TextParser_cfml_Separator) {
                                if (!argToks.empty()) {
                                    node->args.push_back(parseTokensToAST(argToks, cfm_text, sharpContext));
                                    argToks.clear();
                                }
                            } else {
                                argToks.push_back(child);
                            }
                        }
                        if (!argToks.empty()) {
                            node->args.push_back(parseTokensToAST(argToks, cfm_text, sharpContext));
                        }
                    }
                }
                operandStack.push_back(std::move(node));
                nextCanBeUnary = false;
            } else {
                if (!nextCanBeUnary) throw webstrada::exception("Unexpected tokens in expression");
                auto node = std::make_unique<ExprAST>();
                node->type = ExprAST::Variable;
                node->string_val = name;
                operandStack.push_back(std::move(node));
                nextCanBeUnary = false;
            }
        }
        else if (tok.token_id == TextParser_cfml_Parenthesis) {
            if (tok.children.empty()) throw webstrada::exception("Empty parentheses");
            auto ast = parseTokensToAST(tok.children[0].children, cfm_text, sharpContext);
            // Mark the sub-expression as parenthesized so a following index or
            // member access can be rejected like CF does ((arr)[1], (s).a).
            ast->fromParen = true;
            operandStack.push_back(std::move(ast));
            nextCanBeUnary = false;
        }
        else if (tok.token_id == TextParser_cfml_Function) {
            std::string name(cfm_text + tok.position, tok.len);
            size_t paren = name.find('(');
            if (paren != std::string::npos) {
                name = name.substr(0, paren);
            }
            while(!name.empty() && isspace(name.back())) name.pop_back();

            std::string uname = name;
            while(!uname.empty() && isspace(uname.front())) uname.erase(uname.begin());
            while(!uname.empty() && isspace(uname.back())) uname.pop_back();
            for (auto &c : uname) c = toupper(c);

            auto isOperatorWord = [](const std::string &s) {
                return s == "NOT" || s == "AND" || s == "OR" || s == "XOR" || s == "EQV" || s == "IMP" || s == "MOD" ||
                       s == "EQ" || s == "NEQ" || s == "LT" || s == "LTE" || s == "LE" || s == "GT" || s == "GTE" || s == "GE" ||
                       s == "IS" || s == "EQUAL" || s == "CONTAINS" || s == "DOES NOT CONTAIN" || s == "IS NOT";
            };

            // An operator keyword used as a member method (`arr.contains(2)`)
            // is not an operator: the previous token is the '.' ObjectMember,
            // or mergeObjectMembers has left the base as a Variable ending
            // with '.'. Either way the word is a member-method name, not the
            // CFML CONTAINS/EQ/... operator.
            // NOTE: a '.' merely *pending on the operator stack* is NOT member
            // context — it stays there until a lower-precedence operator
            // flushes it, so `s.a AND (...)` (AND arriving as a Function token
            // because a '(' follows) must stay a boolean operator, never
            // `a.AND(...)` which would also orphan the dot's left operand.
            bool operatorAsMember = isOperatorWord(uname) &&
                ((i > 0 && tokens[i - 1].token_id == TextParser_cfml_ObjectMember) ||
                 (!operandStack.empty() && operandStack.back()->type == ExprAST::Variable &&
                  !operandStack.back()->string_val.empty() &&
                  operandStack.back()->string_val.back() == '.'));

            if (isOperatorWord(uname) && !operatorAsMember) {
                bool isUnary = nextCanBeUnary;
                int prec = getOpPrecedence(uname, isUnary);

                while (!opStack.empty()) {
                    auto topOp = opStack.back();
                    int topPrec = getOpPrecedence(topOp.first, topOp.second);

                    if (topPrec > prec || (topPrec == prec && !isRightAssociative(uname))) {
                        applyOp();
                    } else {
                        break;
                    }
                }

                opStack.push_back({uname, isUnary});
                nextCanBeUnary = true;
            } else {
                auto node = std::make_unique<ExprAST>();
                node->type = ExprAST::FuncCall;
                node->op_val = name;

                // A function-call argument `name = value` (a bare identifier
                // assigned a single value at the top level of the argument) is a
                // *named argument*, not an assignment. Convert it to a NamedArg
                // so the call site can bind it by parameter name (CF supports
                // named args; previously the JIT treated it as a positional arg
                // whose value is the RHS — BUGS.md "Named arguments").
                auto maybeNamedArg = [&](std::unique_ptr<ExprAST> ast) -> std::unique_ptr<ExprAST> {
                    if (ast && ast->type == ExprAST::Assign && ast->op_val == "=" &&
                        ast->left && ast->left->type == ExprAST::Variable) {
                        auto na = std::make_unique<ExprAST>();
                        na->type = ExprAST::NamedArg;
                        na->string_val = ast->left->string_val;
                        na->right = std::move(ast->right);
                        return na;
                    }
                    return ast;
                };

                // Merging: Check if followed by Parenthesis (arguments list sibling)
                if (i + 1 < tokens.size() && tokens[i + 1].token_id == TextParser_cfml_Parenthesis) {
                    const auto &subExprTok = tokens[i + 1];
                    i++; // Consume the Parenthesis token

                    if (!subExprTok.children.empty() && (subExprTok.children[0].token_id == TextParser_cfml_Expression || subExprTok.children[0].token_id == TextParser_cfml_ScriptExpression)) {
                        const auto &exprTok = subExprTok.children[0];
                        std::vector<TextParserTokenItem> argToks;
                        for (const auto &child : exprTok.children) {
                            if (child.token_id == TextParser_cfml_Separator) {
                                if (!argToks.empty()) {
                                    node->args.push_back(maybeNamedArg(parseTokensToAST(argToks, cfm_text, sharpContext)));
                                    argToks.clear();
                                }
                            } else {
                                argToks.push_back(child);
                            }
                        }
                        if (!argToks.empty()) {
                            node->args.push_back(maybeNamedArg(parseTokensToAST(argToks, cfm_text, sharpContext)));
                        }
                    }
                } else if (!tok.children.empty() && (tok.children[0].token_id == TextParser_cfml_Expression || tok.children[0].token_id == TextParser_cfml_ScriptExpression)) {
                    const auto &exprTok = tok.children[0];
                    std::vector<TextParserTokenItem> argToks;
                    for (const auto &child : exprTok.children) {
                        if (child.token_id == TextParser_cfml_Separator) {
                            if (!argToks.empty()) {
                                node->args.push_back(maybeNamedArg(parseTokensToAST(argToks, cfm_text, sharpContext)));
                                argToks.clear();
                            }
                        } else {
                            argToks.push_back(child);
                        }
                    }
                    if (!argToks.empty()) {
                        node->args.push_back(maybeNamedArg(parseTokensToAST(argToks, cfm_text, sharpContext)));
                    }
                }
                // Member function call: `base.fn(args)` — mergeObjectMembers has
                // left the base as a Variable ending with '.', or a '.' operator
                // is pending on the stack (inside a function's argument list).
                if (tryMemberCall(node)) {
                    continue;
                }
                operandStack.push_back(std::move(node));
                nextCanBeUnary = false;
            }
        }
        else if (tok.token_id == TextParser_cfml_ArrayIndex) {
            if (nextCanBeUnary) {
                // `[...]` at a position where an operand is expected is an
                // array literal ([10, 20, 30]); otherwise (right after an
                // operand) it is an index operation (arr[i]). CFML has no
                // ambiguity: a literal array never follows a value directly.
                auto node = std::make_unique<ExprAST>();
                node->type = ExprAST::ArrayLiteral;
                if (!tok.children.empty() && (tok.children[0].token_id == TextParser_cfml_Expression || tok.children[0].token_id == TextParser_cfml_ScriptExpression)) {
                    std::vector<TextParserTokenItem> elemToks;
                    for (const auto &child : tok.children[0].children) {
                        if (child.token_id == TextParser_cfml_Separator) {
                            if (!elemToks.empty()) {
                                node->elements.push_back(parseTokensToAST(elemToks, cfm_text, sharpContext));
                                elemToks.clear();
                            }
                        } else {
                            elemToks.push_back(child);
                        }
                    }
                    if (!elemToks.empty()) {
                        node->elements.push_back(parseTokensToAST(elemToks, cfm_text, sharpContext));
                    }
                }
                operandStack.push_back(std::move(node));
                nextCanBeUnary = false;
            } else {
                if (operandStack.empty()) throw webstrada::exception("Index operator missing target array");

                // In CFML, member access binds tighter than indexing: s.k[2]
                // is (s.k)[2], not s[k[2]]. When a '.' operator is pending on
                // the stack (unmerged ObjectMember/Variable sequences, e.g.
                // inside a cfscript function argument list), flush it before
                // binding the index to the top operand.
                if (!opStack.empty()) {
                    int idxPrec = getOpPrecedence(".", false);
                    while (!opStack.empty()) {
                        auto topOp = opStack.back();
                        int topPrec = getOpPrecedence(topOp.first, topOp.second);
                        if (topPrec >= idxPrec && !topOp.second) {
                            applyOp();
                        } else {
                            break;
                        }
                    }
                }

                auto arrNode = std::move(operandStack.back());
                operandStack.pop_back();

                // CF rejects indexing a parenthesized expression ((arr)[1]).
                if (arrNode->fromParen) {
                    throw webstrada::exception("Cannot index a parenthesized expression");
                }

                // A nested index/member on an array object (arr2[3][1]) reports
                // CF's array-object out-of-bounds message, not the named
                // variable form.
                if (arrNode->type == ExprAST::VarIndex ||
                    (arrNode->type == ExprAST::BinaryOp && arrNode->op_val == ".")) {
                    arrNode->isChainedMember = true;
                }

                auto node = std::make_unique<ExprAST>();
                node->type = ExprAST::VarIndex;
                node->left = std::move(arrNode);

                // The indexed target is a chain base: an undefined variable
                // must throw, never fall back to a built-in method handle.
                if (node->left->type == ExprAST::Variable) {
                    node->left->isChainBase = true;
                }

                if (!tok.children.empty() && (tok.children[0].token_id == TextParser_cfml_Expression || tok.children[0].token_id == TextParser_cfml_ScriptExpression)) {
                    node->right = parseTokensToAST(tok.children[0].children, cfm_text, sharpContext);
                } else {
                    throw webstrada::exception("Empty array index");
                }

                // CF also rejects indexing an array *literal* with a simple
                // name reference — a bare variable ([1,2][a]), a parenthesized
                // one ([1,2][(a)]), or a dotted member ([1,2][s.x]). Computed
                // indices ([1,2][a + 0], [1,2][Abs(-1)], [1,2][arr[1]],
                // [1,2][true], [1,2][2]) are accepted. Variable arrays
                // (arr[i]) are unaffected.
                if (node->left->type == ExprAST::ArrayLiteral &&
                    node->right && isSimpleNameChain(node->right.get())) {
                    throw webstrada::exception("Cannot index an array literal with a variable");
                }

                operandStack.push_back(std::move(node));
                nextCanBeUnary = false;
            }
        }
        else if (tok.token_id == TextParser_cfml_CodeBlock) {
            if (nextCanBeUnary) {
                // `{...}` at a position where an operand is expected is a
                // struct literal ({a:1, b:"x"}); ColdFusion accepts both the
                // `key:value` and `key=value` forms.
                auto node = std::make_unique<ExprAST>();
                node->type = ExprAST::StructLiteral;
                std::vector<TextParserTokenItem> pairToks;
                auto flushPair = [&]() {
                    if (pairToks.empty()) return;
                    auto sep = std::find_if(pairToks.begin(), pairToks.end(), [&](const TextParserTokenItem &t) {
                        if (isOperatorToken(t.token_id)) {
                            std::string op(cfm_text + t.position, t.len);
                            while (!op.empty() && isspace(op.front())) op.erase(op.begin());
                            while (!op.empty() && isspace(op.back())) op.pop_back();
                            return op == ":" || op == "=";
                        }
                        return false;
                    });
                    if (sep != pairToks.end()) {
                        std::string key;
                        if (pairToks[0].token_id == TextParser_cfml_DoubleString || pairToks[0].token_id == TextParser_cfml_SingleString) {
                            // Quoted keys keep their casing ({"aB":1} → "aB").
                            key = std::string(cfm_text + pairToks[0].position + 1, pairToks[0].len - 2);
                        } else {
                            // CF uppercases unquoted struct-literal keys ({a:1} → "A").
                            key = std::string(cfm_text + pairToks[0].position, pairToks[0].len);
                            for (auto &c : key) c = toupper(c);
                        }
                        node->structLiteralKeys.push_back(key);
                        std::vector<TextParserTokenItem> valToks(sep + 1, pairToks.end());
                        if (!valToks.empty()) {
                            node->structLiteralValues.push_back(parseTokensToAST(valToks, cfm_text, sharpContext));
                        } else {
                            node->structLiteralValues.push_back(nullptr);
                        }
                    }
                    pairToks.clear();
                };
                if (!tok.children.empty() && (tok.children[0].token_id == TextParser_cfml_Expression || tok.children[0].token_id == TextParser_cfml_ScriptExpression)) {
                    for (const auto &child : tok.children[0].children) {
                        if (child.token_id == TextParser_cfml_Separator) {
                            flushPair();
                        } else {
                            pairToks.push_back(child);
                        }
                    }
                }
                flushPair();
                operandStack.push_back(std::move(node));
                nextCanBeUnary = false;
            }
        }
        else if (tok.token_id == TextParser_cfml_ObjectMember) {
            std::string op = ".";
            bool isUnary = false;
            int prec = getOpPrecedence(op, isUnary);

            while (!opStack.empty()) {
                auto topOp = opStack.back();
                int topPrec = getOpPrecedence(topOp.first, topOp.second);

                if (topPrec > prec || (topPrec == prec && !isRightAssociative(op))) {
                    applyOp();
                } else {
                    break;
                }
            }

            opStack.push_back({op, isUnary});
            nextCanBeUnary = true;
        }
        else if (isOperatorToken(tok.token_id)) {
            std::string op(cfm_text + tok.position, tok.len);
            while(!op.empty() && isspace(op.front())) op.erase(op.begin());
            while(!op.empty() && isspace(op.back())) op.pop_back();
            std::string rawOp = op;
            for (auto &c : op) c = toupper(c);

            // An operator keyword used as a member method (`arr.contains(2)`)
            // or a member property (`cacheresult.contains`, `q.eq`) arrives as
            // an operator token (CompareOperator etc.), but directly after an
            // incomplete member access — the previous token is the '.', or the
            // top operand is a merged Variable ending with '.' — it is a
            // member name, never the CFML CONTAINS/EQ/... operator: a binary
            // operator can never be the right-hand side of a pending dot.
            // NOTE: a '.' merely *pending on the operator stack* is not enough
            // context here; it stays there until a lower-precedence operator
            // flushes it, so `s.a AND (...)` must not look like member access.
            {
                bool opWord = op == "NOT" || op == "AND" || op == "OR" || op == "XOR" ||
                              op == "EQV" || op == "IMP" || op == "MOD" ||
                              op == "EQ" || op == "NEQ" || op == "LT" || op == "LTE" || op == "LE" ||
                              op == "GT" || op == "GTE" || op == "GE" || op == "IS" || op == "EQUAL" ||
                              op == "CONTAINS" || op == "DOES NOT CONTAIN" || op == "IS NOT";
                bool afterMemberDot =
                    (i > 0 && tokens[i - 1].token_id == TextParser_cfml_ObjectMember) ||
                    (!operandStack.empty() && operandStack.back()->type == ExprAST::Variable &&
                     !operandStack.back()->string_val.empty() &&
                     operandStack.back()->string_val.back() == '.');
                if (opWord && afterMemberDot && i + 1 < tokens.size() &&
                    tokens[i + 1].token_id == TextParser_cfml_Parenthesis) {
                    auto node = std::make_unique<ExprAST>();
                    node->type = ExprAST::FuncCall;
                    node->op_val = op;
                    const auto &subExprTok = tokens[i + 1];
                    i++; // consume the Parenthesis token
                    if (!subExprTok.children.empty() &&
                        (subExprTok.children[0].token_id == TextParser_cfml_Expression ||
                         subExprTok.children[0].token_id == TextParser_cfml_ScriptExpression)) {
                        const auto &exprTok = subExprTok.children[0];
                        std::vector<TextParserTokenItem> argToks;
                        for (const auto &child : exprTok.children) {
                            if (child.token_id == TextParser_cfml_Separator) {
                                if (!argToks.empty()) {
                                    node->args.push_back(parseTokensToAST(argToks, cfm_text, sharpContext));
                                    argToks.clear();
                                }
                            } else {
                                argToks.push_back(child);
                            }
                        }
                        if (!argToks.empty()) {
                            node->args.push_back(parseTokensToAST(argToks, cfm_text, sharpContext));
                        }
                    }
                    if (tryMemberCall(node)) {
                        continue;
                    }
                    operandStack.push_back(std::move(node));
                    nextCanBeUnary = false;
                    continue;
                }
                // No parentheses: an operator keyword directly after a dot is
                // a bare property name (`cacheresult.contains`, `q.eq`). The
                // original casing is kept — CF struct keys preserve how they
                // were first written.
                if (opWord && afterMemberDot) {
                    if (!operandStack.empty() && operandStack.back()->type == ExprAST::Variable &&
                        !operandStack.back()->string_val.empty() &&
                        operandStack.back()->string_val.back() == '.') {
                        // Merged base (`mergeObjectMembers` produced e.g. "s ."):
                        // fold the property into the base ("s . mod").
                        std::string &base = operandStack.back()->string_val;
                        while (!base.empty() && isspace((unsigned char)base.back())) base.pop_back();
                        base.pop_back(); // trailing '.'
                        while (!base.empty() && isspace((unsigned char)base.back())) base.pop_back();
                        base += "." + rawOp;
                    } else {
                        // Unmerged sequence (previous token is the '.'): push a
                        // plain variable operand and leave the pending '.' on
                        // the operator stack for the normal flush.
                        auto vnode = std::make_unique<ExprAST>();
                        vnode->type = ExprAST::Variable;
                        vnode->string_val = rawOp;
                        operandStack.push_back(std::move(vnode));
                    }
                    nextCanBeUnary = false;
                    continue;
                }
            }

            // ++ / -- increment and decrement. Supports both single token (new textparser)
            // and two adjacent sign operators. x++ / ++x evaluate to old / new value.
            bool isSingleTokenIncDec = (op == "++" || op == "--");
            bool isTwoTokenIncDec = false;
            if (!isSingleTokenIncDec && (op == "+" || op == "-") &&
                i + 1 < tokens.size() && isOperatorToken(tokens[i + 1].token_id)) {
                std::string op2(cfm_text + tokens[i + 1].position, tokens[i + 1].len);
                while(!op2.empty() && isspace(op2.front())) op2.erase(op2.begin());
                while(!op2.empty() && isspace(op2.back())) op2.pop_back();
                isTwoTokenIncDec = (op2 == op);
            }

            if (isSingleTokenIncDec || isTwoTokenIncDec) {
                std::string baseOp = isSingleTokenIncDec ? op.substr(0, 1) : op;
                if (nextCanBeUnary) {
                    // Pre-increment/dec: ++x (the operand is a plain variable).
                    size_t varIdx = isSingleTokenIncDec ? (i + 1) : (i + 2);
                    if (varIdx >= tokens.size() ||
                        tokens[varIdx].token_id != TextParser_cfml_Variable) {
                        throw webstrada::exception("Increment/decrement requires a variable");
                    }
                    const auto &operandTok = tokens[varIdx];
                    auto varNode = std::make_unique<ExprAST>();
                    varNode->type = ExprAST::Variable;
                    varNode->string_val = std::string(cfm_text + operandTok.position, operandTok.len);
                    varNode->token = operandTok;
                    auto incNode = std::make_unique<ExprAST>();
                    incNode->type = ExprAST::Increment;
                    incNode->op_val = baseOp;
                    incNode->isPre = true;
                    incNode->left = std::move(varNode);
                    operandStack.push_back(std::move(incNode));
                    i = varIdx; // consume both operator and the variable
                    nextCanBeUnary = false;
                    continue;
                } else {
                    // Post-increment/dec: x++
                    if (operandStack.empty()) throw webstrada::exception("Increment/decrement missing operand");
                    auto varNode = std::move(operandStack.back());
                    operandStack.pop_back();
                    auto incNode = std::make_unique<ExprAST>();
                    incNode->type = ExprAST::Increment;
                    incNode->op_val = baseOp;
                    incNode->isPre = false;
                    incNode->left = std::move(varNode);
                    operandStack.push_back(std::move(incNode));
                    if (isTwoTokenIncDec) {
                        i++; // consume the second sign operator
                    }
                    nextCanBeUnary = false;
                    continue;
                }
            }

            // Inside a #...# interpolation (a cfoutput body expression or a
            // string-literal interpolation) CF rejects the symbolic < > <= >=
            // comparison operators — its tag parser consumes < and > as tag
            // delimiters — so they must not be accepted leniently here. They
            // remain valid in direct cfscript expressions (sharpContext=false).
            if (sharpContext && (op == "<" || op == ">" || op == "<=" || op == ">=")) {
                throw webstrada::exception("Illegal symbolic comparison operator inside #...# (use the word forms GT, LT, GTE or LTE)");
            }

            // Ternary `? :` (cfscript). The `?` splits `cond ? then : else`; the
            // then-expression runs up to a top-level ':' and the else is the
            // remainder of the current token list. The condition is the current
            // top operand (after pending higher-precedence operators apply).
            if (op == "?" && !nextCanBeUnary) {
                // Apply pending operators with higher precedence than '?' so the
                // condition operand is fully reduced before we split.
                while (!opStack.empty()) {
                    auto topOp = opStack.back();
                    int topPrec = getOpPrecedence(topOp.first, topOp.second);
                    if (topPrec > 1) {
                        applyOp();
                    } else {
                        break;
                    }
                }
                if (operandStack.empty()) throw webstrada::exception("Ternary operator missing condition");
                auto condNode = std::move(operandStack.back());
                operandStack.pop_back();

                // Find the matching ':' that closes this ternary, tracking nested
                // parens/brackets and nested '?'. The tokens after ':' (up to the
                // end of this token list, i.e. the current statement) are the else.
                size_t colonIdx = (size_t)-1;
                int depth = 0;
                for (size_t j = i + 1; j < tokens.size(); j++) {
                    const auto &t = tokens[j];
                    if (t.token_id == TextParser_cfml_Parenthesis ||
                        t.token_id == TextParser_cfml_CodeBlock ||
                        t.token_id == TextParser_cfml_ArrayIndex) {
                        // A nested group; its children are self-contained.
                        continue;
                    }
                    if (isOperatorToken(t.token_id)) {
                        std::string o(cfm_text + t.position, t.len);
                        while (!o.empty() && isspace(o.front())) o.erase(o.begin());
                        while (!o.empty() && isspace(o.back())) o.pop_back();
                        for (auto &c : o) c = toupper(c);
                        if (o == "?") {
                            depth++;
                        } else if (o == ":") {
                            if (depth == 0) { colonIdx = j; break; }
                            depth--;
                        }
                    }
                }
                if (colonIdx == (size_t)-1) {
                    throw webstrada::exception("Ternary operator missing ':'");
                }

                std::vector<TextParserTokenItem> thenToks(tokens.begin() + i + 1, tokens.begin() + colonIdx);
                std::vector<TextParserTokenItem> elseToks(tokens.begin() + colonIdx + 1, tokens.end());
                auto thenNode = parseTokensToAST(thenToks, cfm_text, sharpContext);
                auto elseNode = parseTokensToAST(elseToks, cfm_text, sharpContext);

                auto ternNode = std::make_unique<ExprAST>();
                ternNode->type = ExprAST::TernaryExpr;
                ternNode->left = std::move(condNode);
                ternNode->right = std::move(thenNode);
                ternNode->args.push_back(std::move(elseNode));
                operandStack.push_back(std::move(ternNode));
                i = tokens.size(); // the else consumed the rest of the list
                nextCanBeUnary = false;
                continue;
            }

            bool isUnary = nextCanBeUnary;
            int prec = getOpPrecedence(op, isUnary);

            while (!opStack.empty()) {
                auto topOp = opStack.back();
                int topPrec = getOpPrecedence(topOp.first, topOp.second);

                if (topPrec > prec || (topPrec == prec && !isRightAssociative(op))) {
                    applyOp();
                } else {
                    break;
                }
            }

            opStack.push_back({op, isUnary});
            nextCanBeUnary = true;
        }
        else if (tok.token_id == TextParser_cfml_SharpExpression) {
            // A #...# -wrapped value inside a plain expression (<cfset x = #expr#>,
            // CFKillBoard parser.cfm uses <cfset VictimID=#getPlayerID(...)#>):
            // CF ignores the surrounding hashes and evaluates the inner
            // expression. Unwrap it and push it as an operand. (The same token
            // inside a string literal is handled by CompileExprAST's String
            // case; here it is a value position.)
            if (!nextCanBeUnary) throw webstrada::exception("Unexpected tokens in expression");
            std::vector<TextParserTokenItem> exprToks;
            for (const auto &sub : tok.children) {
                if (sub.token_id == TextParser_cfml_Expression) {
                    for (const auto &t : sub.children) exprToks.push_back(t);
                } else if (sub.token_id != TextParser_cfml_ExpressionEnd) {
                    exprToks.push_back(sub);
                }
            }
            if (!exprToks.empty()) {
                operandStack.push_back(parseTokensToAST(exprToks, cfm_text, sharpContext));
                nextCanBeUnary = false;
            }
        }
        else if (tok.token_id == TextParser_cfml_Keyword) {
            std::string kwText(cfm_text + tok.position, tok.len);
            while (!kwText.empty() && isspace(kwText.front())) kwText.erase(kwText.begin());
            while (!kwText.empty() && isspace(kwText.back())) kwText.pop_back();
            for (auto &c : kwText) c = tolower(c);

            if (kwText == "function" && nextCanBeUnary &&
                i + 2 < tokens.size() &&
                tokens[i + 1].token_id == TextParser_cfml_Parenthesis &&
                tokens[i + 2].token_id == TextParser_cfml_CodeBlock) {
                // Anonymous function (closure): `function(params) { body }`.
                auto node = std::make_unique<ExprAST>();
                node->type = ExprAST::Closure;
                parseParamList(tokens[i + 1], cfm_text, node->closureParams,
                               node->closureParamTypes, node->closureParamDefaults);
                const auto &cb = tokens[i + 2];
                if (!cb.children.empty() && cb.children[0].token_id == TextParser_cfml_ScriptExpression) {
                    node->closureBody = cb.children[0].children;
                } else {
                    node->closureBody = cb.children;
                }
                operandStack.push_back(std::move(node));
                i += 2; // consume the Parenthesis and CodeBlock
                nextCanBeUnary = false;
                continue;
            }
            if (kwText == "var") {
                // The textparser tokenizes `var` as a Keyword unconditionally,
                // but CF 2025 accepts it in three non-declaration roles:
                //  1. a member name after '.' (`arguments.var`),
                //  2. a named-argument name (`two(var="x")`, `trace(var="x")`),
                //  3. a struct/member key in `foo.var` / `{var: 1}`.
                // In each case treat it as a variable so the surrounding code
                // can consume it. A real declaration is `var x = ..` (Keyword,
                // then a Variable), never `var = ..` or `.var`.
                bool memberContext = (!opStack.empty() && opStack.back().first == "." && !opStack.back().second);
                bool namedArg = (i + 1 < tokens.size() &&
                                 isOperatorToken(tokens[i + 1].token_id));
                if (namedArg) {
                    std::string opText(cfm_text + tokens[i + 1].position, tokens[i + 1].len);
                    while (!opText.empty() && isspace(opText.front())) opText.erase(opText.begin());
                    while (!opText.empty() && isspace(opText.back())) opText.pop_back();
                    namedArg = (opText == "=");
                }
                if (memberContext || namedArg) {
                    auto node = std::make_unique<ExprAST>();
                    node->type = ExprAST::Variable;
                    node->string_val = "var";
                    operandStack.push_back(std::move(node));
                    nextCanBeUnary = false;
                    continue;
                }
                // CF rejects `var` outside a function body (also inside an
                // included template, compiled as a standalone page). Inside a
                // function body the keyword is dropped and the assignment that
                // follows targets the function's local scope.
                if (!g_compileInFunctionBody) {
                    std::string vname = "";
                    if (i + 1 < tokens.size() && tokens[i + 1].token_id == TextParser_cfml_Variable) {
                        vname = std::string(cfm_text + tokens[i + 1].position, tokens[i + 1].len);
                    }
                    throw webstrada::exception(webstrada::string(("The local variable " + vname + " cannot be declared outside of a function.").c_str()));
                }
                continue;
            }
            if (!nextCanBeUnary) {
                webstrada::string text(cfm_text + tok.position, tok.len);
                text = text.trimmed();
                throw webstrada::exception("Unexpected keyword '" + text + "' in expression");
            }

            // `this` is a scope reference (component this scope / CF: page `this`
            // is undefined). Treat it as a variable so `this.x` resolves at
            // runtime through the scope lookup.
            if (kwText == "this") {
                auto node = std::make_unique<ExprAST>();
                node->type = ExprAST::Variable;
                node->string_val = kwText;
                operandStack.push_back(std::move(node));
                nextCanBeUnary = false;
            }
        }
    }

    while (!opStack.empty()) {
        applyOp();
    }

    if (operandStack.empty()) {
        auto nullNode = std::make_unique<ExprAST>();
        nullNode->type = ExprAST::LiteralNull;
        return nullNode;
    }

    return std::move(operandStack.back());
}

llvm::Value *CompileExprAST(
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *function,
    const std::unique_ptr<ExprAST> &node,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie,
    llvm::Value *application, llvm::Value *session,
    llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const char *cfm_text) {
    if (!node) return nullptr;

    auto getPtrGlobalString = [&](const std::string &s) {
        return builder.CreateGlobalString(s, "", 0, module, true);
    };

    switch (node->type) {
    case ExprAST::LiteralNull: {
        auto *f = module->getFunction("cfvariant_create_null");
        if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {}, false), llvm::Function::InternalLinkage, "cfvariant_create_null", module);
        return emitCall(builder, f, {});
    }
    case ExprAST::LiteralInt: {
        auto *f = module->getFunction("cfvariant_create_int");
        if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty()}, false), llvm::Function::InternalLinkage, "cfvariant_create_int", module);
        return emitCall(builder, f, {builder.getInt32(node->int_val)});
    }
    case ExprAST::LiteralLong: {
        auto *f = module->getFunction("cfvariant_create_long");
        if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt64Ty()}, false), llvm::Function::InternalLinkage, "cfvariant_create_long", module);
        return emitCall(builder, f, {builder.getInt64(node->long_val)});
    }
    case ExprAST::LiteralFloat: {
        auto *f = module->getFunction("cfvariant_create_float_literal");
        if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getDoubleTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_float_literal", module);
        return emitCall(builder, f, {getPtrGlobalString(node->float_literal_text), llvm::ConstantFP::get(builder.getDoubleTy(), node->float_val)});
    }
    case ExprAST::LiteralBool: {
        auto *f = module->getFunction("cfvariant_create_bool_literal");
        if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt1Ty()}, false), llvm::Function::InternalLinkage, "cfvariant_create_bool_literal", module);
        return emitCall(builder, f, {builder.getInt1(node->bool_val)});
    }
    case ExprAST::ArrayLiteral: {
        auto *fArr = module->getFunction("cfvariant_create_array");
        if (!fArr) fArr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {}, false), llvm::Function::InternalLinkage, "cfvariant_create_array", module);
        llvm::Value *arrVal = emitCall(builder, fArr, {});
        auto *fSet = module->getFunction("cfvariant_index_assign");
        if (!fSet) fSet = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index_assign", module);
        auto *fIdx = module->getFunction("cfvariant_create_int");
        if (!fIdx) fIdx = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty()}, false), llvm::Function::InternalLinkage, "cfvariant_create_int", module);
        for (size_t i = 0; i < node->elements.size(); i++) {
            auto *elemVal = CompileExprAST(module, builder, function, node->elements[i], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *idxVal = emitCall(builder, fIdx, {builder.getInt32(static_cast<int>(i + 1))});
            emitCall(builder, fSet, {arrVal, idxVal, elemVal});
        }
        return arrVal;
    }
    case ExprAST::StructLiteral: {
        auto *fSt = module->getFunction("cfvariant_create_struct");
        if (!fSt) fSt = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {}, false), llvm::Function::InternalLinkage, "cfvariant_create_struct", module);
        llvm::Value *structVal = emitCall(builder, fSt, {});
        auto *fSet = module->getFunction("cfvariant_index_assign");
        if (!fSet) fSet = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index_assign", module);
        auto *fStr = module->getFunction("cfvariant_create_string");
        if (!fStr) fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
        for (size_t i = 0; i < node->structLiteralKeys.size(); i++) {
            llvm::Value *valVal = node->structLiteralValues[i]
                ? CompileExprAST(module, builder, function, node->structLiteralValues[i], cgi, server, cookie, application, session, url, form, variables, cfm_text)
                : nullptr;
            if (!valVal) {
                auto *fNull = module->getFunction("cfvariant_create_null");
                if (!fNull) fNull = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {}, false), llvm::Function::InternalLinkage, "cfvariant_create_null", module);
                valVal = emitCall(builder, fNull, {});
            }
            auto *keyVal = emitCall(builder, fStr, {getPtrGlobalString(node->structLiteralKeys[i])});
            emitCall(builder, fSet, {structVal, keyVal, valVal});
        }
        return structVal;
    }
    case ExprAST::LiteralString: {
        const auto &tok = node->token;
        if (tok.children.empty()) {
            std::string text(cfm_text + tok.position, tok.len);
            if (text.length() >= 2) {
                text = text.substr(1, text.length() - 2);
            }
            auto *f = module->getFunction("cfvariant_create_string");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
            return emitCall(builder, f, {getPtrGlobalString(text)});
        } else {
            size_t segStart = tok.position + 1;
            llvm::Value *resStrVal = nullptr;

            auto appendSeg = [&](llvm::Value *nextVal) {
                if (!resStrVal) {
                    resStrVal = nextVal;
                } else {
                    auto *f = module->getFunction("cfvariant_concat");
                    if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_concat", module);
                    resStrVal = emitCall(builder, f, {resStrVal, nextVal});
                }
            };

            for (const auto &child : tok.children) {
                if (child.position > segStart) {
                    std::string plain(cfm_text + segStart, child.position - segStart);
                    auto *f = module->getFunction("cfvariant_create_string");
                    if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                    appendSeg(emitCall(builder, f, {getPtrGlobalString(plain)}));
                }

                if (child.token_id == TextParser_cfml_SharpChar) {
                    auto *f = module->getFunction("cfvariant_create_string");
                    if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                    appendSeg(emitCall(builder, f, {getPtrGlobalString("#")}));
                } else if (child.token_id == TextParser_cfml_DoubleChar) {
                    // Escaped double-quote inside a double-quoted literal: "" -> "
                    auto *f = module->getFunction("cfvariant_create_string");
                    if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                    appendSeg(emitCall(builder, f, {getPtrGlobalString("\"")}));
                } else if (child.token_id == TextParser_cfml_SingleChar) {
                    // Escaped single-quote inside a single-quoted literal: '' -> '
                    auto *f = module->getFunction("cfvariant_create_string");
                    if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                    appendSeg(emitCall(builder, f, {getPtrGlobalString("'")}));
                } else if (child.token_id == TextParser_cfml_SharpExpression) {
                    // The SharpExpression's Expression group is deleted when it
                    // wraps a single child (deleteIfOnlyOneChild in
                    // cfml_definition.json), so a bare variable/number/boolean/
                    // parenthesized expression appears as a direct child. Build
                    // the token list from either the Expression group's children
                    // or the direct children so #x# interpolates correctly.
                    std::vector<TextParserTokenItem> exprToks;
                    for (const auto &sub : child.children) {
                        if (sub.token_id == TextParser_cfml_Expression) {
                            for (const auto &t : sub.children) exprToks.push_back(t);
                        } else if (sub.token_id != TextParser_cfml_ExpressionEnd) {
                            exprToks.push_back(sub);
                        }
                    }
                    if (!exprToks.empty()) {
                        auto exprAST = parseTokensToAST(exprToks, cfm_text, true);
                        auto *exprVal = CompileExprAST(module, builder, function, exprAST, cgi, server, cookie, application, session, url, form, variables, cfm_text);
                        appendSeg(exprVal);
                    }
                }
                segStart = child.position + child.len;
            }

            size_t segEnd = tok.position + tok.len - 1;
            if (segEnd > segStart) {
                std::string plain(cfm_text + segStart, segEnd - segStart);
                auto *f = module->getFunction("cfvariant_create_string");
                if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                appendSeg(emitCall(builder, f, {getPtrGlobalString(plain)}));
            }

            if (!resStrVal) {
                auto *f = module->getFunction("cfvariant_create_string");
                if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                resStrVal = emitCall(builder, f, {getPtrGlobalString("")});
            }
            return resStrVal;
        }
    }
    case ExprAST::Variable: {
        // A bare identifier that is not a variable resolves to a built-in
        // method handle (coldfusion.runtime.CFPageMethod@<hash>) instead of
        // throwing (previously BUGS.md #2). A variable shadows the built-in,
        // and a chain base (s.x / arr[1]) never falls back to a handle.
        //
        // Compile-time variable binding (ColdFusion's "fast path"): at page
        // level (not inside a UDF body), a simple identifier read is routed
        // through cfvariant_get_var_fast / cfvariant_bare_identifier_fast with
        // a per-name slot. The slot memoizes the resolved variables-scope
        // pointer on the first read; subsequent reads short-circuit without a
        // scope search (generation-guarded against StructDelete/StructClear).
        // Dotted/member names and built-in scope names are excluded.
        const std::string &nm = node->string_val;
        bool simplePageVar = !g_compileInFunctionBody && !nm.empty();
        if (simplePageVar) {
            for (char c : nm) {
                if (c == '.' || c == '[' || c == '(') { simplePageVar = false; break; }
            }
        }
        if (simplePageVar) {
            static const std::set<std::string> scopeNames = {
                "VARIABLES","CGI","URL","FORM","COOKIE","SERVER","APPLICATION",
                "SESSION","REQUEST","CLIENT","THIS","LOCAL","ARGUMENTS","SUPER",
                "CFLOCATION","FILE","HTTP","PDF"};
            std::string up = nm;
            for (auto &c : up) c = (char)toupper((unsigned char)c);
            if (scopeNames.count(up)) simplePageVar = false;
        }
        if (simplePageVar) {
            llvm::Value *slot = nullptr;
            std::pair<llvm::Function*, std::string> slotKey(function, nm);
            auto it = g_varFastSlots.find(slotKey);
            if (it != g_varFastSlots.end()) {
                slot = it->second;
            } else {
                auto *slotTy = llvm::StructType::get(module->getContext(),
                    {builder.getPtrTy(), builder.getPtrTy(), builder.getInt64Ty()}, false);
                slot = createEntryAlloca(builder, function, slotTy);
                // Zero-init in the function entry block (runs once per request,
                // before any read site).
                llvm::IRBuilderBase::InsertPoint ip = builder.saveIP();
                builder.SetInsertPoint(&function->getEntryBlock(), function->getEntryBlock().begin());
                builder.CreateStore(llvm::ConstantAggregateZero::get(slotTy), slot);
                builder.restoreIP(ip);
                g_varFastSlots[slotKey] = slot;
            }
            auto *f = module->getFunction(node->isChainBase ? "cfvariant_get_var_fast" : "cfvariant_bare_identifier_fast");
            if (!f) {
                std::vector<llvm::Type*> p(10, builder.getPtrTy());
                f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), p, false), llvm::Function::InternalLinkage, node->isChainBase ? "cfvariant_get_var_fast" : "cfvariant_bare_identifier_fast", module);
            }
            llvm::Value *args[] = {slot, cgi, server, cookie, application, session, url, form, variables, getPtrGlobalString(nm)};
            return emitCall(builder, f, args);
        }
        auto *f = module->getFunction(node->isChainBase ? "cfvariant_get_var" : "cfvariant_bare_identifier");
        if (!f) {
            std::vector<llvm::Type*> p(9, builder.getPtrTy());
            f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), p, false), llvm::Function::InternalLinkage, node->isChainBase ? "cfvariant_get_var" : "cfvariant_bare_identifier", module);
        }
        llvm::Value *args[] = {cgi, server, cookie, application, session, url, form, variables, getPtrGlobalString(node->string_val)};
        return emitCall(builder, f, args);
    }
    case ExprAST::UnaryOp: {
        auto *operand = CompileExprAST(module, builder, function, node->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
        if (node->op_val == "-" || node->op_val == "NEG") {
            auto *f = module->getFunction("cfvariant_neg");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_neg", module);
            return emitCall(builder, f, {operand});
        } else if (node->op_val == "NOT" || node->op_val == "!") {
            auto *f = module->getFunction("cfvariant_not");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_not", module);
            return emitCall(builder, f, {operand});
        }
        return operand;
    }
    case ExprAST::BinaryOp: {
        std::string op = node->op_val;

        if (op == ".") {
            // A terminal `.key` on a chain base resolves via cfvariant_get_member
            // so an undefined base reports CF's ELEMENT message ("Element KEY is
            // undefined in BASE."), matching CF 2025 (was BUGS.md
            // "chain-base lookups"). A chained or bracket access keeps the
            // variable message.
            if (node->left->type == ExprAST::Variable && node->left->isChainBase &&
                node->right->type == ExprAST::Variable && !node->isChainedMember) {
                auto *fM = module->getFunction("cfvariant_get_member");
                if (!fM) {
                    std::vector<llvm::Type*> p(11, builder.getPtrTy());
                    fM = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), p, false), llvm::Function::InternalLinkage, "cfvariant_get_member", module);
                }
                llvm::Value *mArgs[] = {cgi, server, cookie, application, session, url, form, variables,
                                        getPtrGlobalString(node->left->string_val),
                                        getPtrGlobalString(node->right->string_val)};
                return emitCall(builder, fM, mArgs);
            }
        }

        auto *lhs = CompileExprAST(module, builder, function, node->left, cgi, server, cookie, application, session, url, form, variables, cfm_text);

        // CF short-circuits AND/OR: the right operand is evaluated only when
        // needed (verified against CF 2025: `true OR undefinedvar` -> true,
        // `lastIndex EQ 0 OR arr[lastIndex]...` with lastIndex=0 never touches
        // arr[0]). Eagerly evaluating both operands threw on the skipped side
        // (was MangoBlog's Queue.cfc:30 "element at position 0 ... cannot be
        // found"). Use LLVM control flow like the ternary so the rhs is
        // compiled but only *executed* on the needed branch. The result
        // semantics match cfvariant_and/cfvariant_or: the first operand when
        // it decides the result, otherwise the second (verified against CF).
        if (op == "AND" || op == "&&" || op == "OR" || op == "||") {
            bool isAnd = (op == "AND" || op == "&&");
            auto *isTruthyFunc = module->getFunction("cfvariant_is_truthy");
            if (!isTruthyFunc) isTruthyFunc = llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_is_truthy", module);
            llvm::Value *lhsTruthy = emitCall(builder, isTruthyFunc, {lhs});
            llvm::Value *isTrue = builder.CreateICmpNE(lhsTruthy, builder.getInt32(0));

            llvm::Function *fn = builder.GetInsertBlock()->getParent();
            auto &llvmCtx = module->getContext();
            auto *rhsBB = llvm::BasicBlock::Create(llvmCtx, isAnd ? "and.rhs" : "or.rhs", fn);
            auto *shortBB = llvm::BasicBlock::Create(llvmCtx, isAnd ? "and.short" : "or.short", fn);
            auto *mergeBB = llvm::BasicBlock::Create(llvmCtx, isAnd ? "and.merge" : "or.merge", fn);
            // AND: rhs only when lhs truthy; OR: rhs only when lhs falsy.
            if (isAnd) builder.CreateCondBr(isTrue, rhsBB, shortBB);
            else       builder.CreateCondBr(isTrue, shortBB, rhsBB);

            builder.SetInsertPoint(rhsBB);
            llvm::Value *rhsVal = CompileExprAST(module, builder, function, node->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::BasicBlock *rhsCont = builder.GetInsertBlock();
            builder.CreateBr(mergeBB);

            builder.SetInsertPoint(shortBB);
            llvm::BasicBlock *shortCont = builder.GetInsertBlock();
            builder.CreateBr(mergeBB);

            builder.SetInsertPoint(mergeBB);
            auto *phi = builder.CreatePHI(builder.getPtrTy(), 2);
            if (isAnd) { phi->addIncoming(rhsVal, rhsCont); phi->addIncoming(lhs, shortCont); }
            else       { phi->addIncoming(lhs, shortCont);  phi->addIncoming(rhsVal, rhsCont); }
            return phi;
        }

        if (op == ".") {
            // A dotted member name on the right ("k2.k3") comes from
            // mergeObjectMembers gluing a chain whose head was an index access
            // (s1[key].k2.k3): emit one hop per segment instead of a single
            // literal-key lookup that always misses (BUGS.md "chained member
            // access after an index hop").
            if (node->right->type == ExprAST::Variable &&
                node->right->string_val.find('.') != std::string::npos) {
                return emitMemberWalk(module, builder, lhs, splitMemberPath(node->right->string_val));
            }
            llvm::Value *keyVal = nullptr;
            if (node->right->type == ExprAST::Variable) {
                auto *fStr = module->getFunction("cfvariant_create_string");
                if (!fStr) fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                keyVal = emitCall(builder, fStr, {getPtrGlobalString(node->right->string_val)});
            } else {
                keyVal = CompileExprAST(module, builder, function, node->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *fIdx = module->getFunction("cfvariant_index");
            if (!fIdx) fIdx = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index", module);
            return emitCall(builder, fIdx, {lhs, keyVal});
        }

        auto *rhs = CompileExprAST(module, builder, function, node->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
        if (op == "+") {
            auto *f = module->getFunction("cfvariant_add");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_add", module);
            return emitCall(builder, f, {lhs, rhs});
        } else if (op == "-") {
            auto *f = module->getFunction("cfvariant_sub");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_sub", module);
            return emitCall(builder, f, {lhs, rhs});
        } else if (op == "*") {
            auto *f = module->getFunction("cfvariant_mul");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_mul", module);
            return emitCall(builder, f, {lhs, rhs});
        } else if (op == "/") {
            auto *f = module->getFunction("cfvariant_div");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_div", module);
            return emitCall(builder, f, {lhs, rhs});
        } else if (op == "MOD" || op == "%") {
            auto *f = module->getFunction("cfvariant_mod");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_mod", module);
            return emitCall(builder, f, {lhs, rhs});
        } else if (op == "\\") {
            auto *f = module->getFunction("cfvariant_idiv");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_idiv", module);
            return emitCall(builder, f, {lhs, rhs});
        } else if (op == "^") {
            auto *f = module->getFunction("cfvariant_pow");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_pow", module);
            return emitCall(builder, f, {lhs, rhs});
        } else if (op == "&") {
            auto *f = module->getFunction("cfvariant_concat");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_concat", module);
            return emitCall(builder, f, {lhs, rhs});
        } else if (op == "XOR") {
            auto *f = module->getFunction("cfvariant_xor");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_xor", module);
            return emitCall(builder, f, {lhs, rhs});
        } else {
            auto *f = module->getFunction("cfvariant_compare");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_compare", module);
            return emitCall(builder, f, {lhs, rhs, getPtrGlobalString(op)});
        }
    }
    case ExprAST::TernaryExpr: {
        // `cond ? then : else`. CF evaluates only the taken branch
        // (EvaluateEngine.evaluateHook), so use LLVM control flow rather than
        // evaluating both. The condition uses CF's _isTruthyValue truthiness.
        auto *condVal = CompileExprAST(module, builder, function, node->left, cgi, server, cookie, application, session, url, form, variables, cfm_text);
        auto *fTruthy = module->getFunction("cf_is_truthy_value");
        if (!fTruthy) fTruthy = llvm::Function::Create(llvm::FunctionType::get(builder.getInt1Ty(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cf_is_truthy_value", module);
        llvm::Value *taken = emitCall(builder, fTruthy, {condVal});

        llvm::Function *fn = builder.GetInsertBlock()->getParent();
        auto &llvmCtx = module->getContext();
        auto *thenBB = llvm::BasicBlock::Create(llvmCtx, "ternary.then", fn);
        auto *elseBB = llvm::BasicBlock::Create(llvmCtx, "ternary.else", fn);
        auto *mergeBB = llvm::BasicBlock::Create(llvmCtx, "ternary.merge", fn);
        builder.CreateCondBr(taken, thenBB, elseBB);

        builder.SetInsertPoint(thenBB);
        llvm::Value *thenResult = CompileExprAST(module, builder, function, node->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
        llvm::BasicBlock *thenCont = builder.GetInsertBlock();
        builder.CreateBr(mergeBB);

        builder.SetInsertPoint(elseBB);
        const std::unique_ptr<ExprAST> &elseNode = node->args.empty() ? node->right : node->args[0];
        llvm::Value *elseResult = CompileExprAST(module, builder, function, elseNode, cgi, server, cookie, application, session, url, form, variables, cfm_text);
        llvm::BasicBlock *elseCont = builder.GetInsertBlock();
        builder.CreateBr(mergeBB);

        builder.SetInsertPoint(mergeBB);
        auto *phi = builder.CreatePHI(builder.getPtrTy(), 2);
        phi->addIncoming(thenResult, thenCont);
        phi->addIncoming(elseResult, elseCont);
        return phi;
    }
    case ExprAST::Assign: {
        // Compound assignment (i += 2, a -= 1, s &= "x", ...) reads the current
        // left-hand value, applies the operator, and assigns the result.
        auto compoundOp = [&](llvm::Value *lhsVal, llvm::Value *rhsVal) -> llvm::Value* {
            auto *fAdd = module->getFunction("cfvariant_add");
            if (!fAdd) fAdd = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_add", module);
            auto *fSub = module->getFunction("cfvariant_sub");
            if (!fSub) fSub = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_sub", module);
            auto *fMul = module->getFunction("cfvariant_mul");
            if (!fMul) fMul = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_mul", module);
            auto *fDiv = module->getFunction("cfvariant_div");
            if (!fDiv) fDiv = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_div", module);
            auto *fMod = module->getFunction("cfvariant_mod");
            if (!fMod) fMod = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_mod", module);
            auto *fConcat = module->getFunction("cfvariant_concat");
            if (!fConcat) fConcat = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_concat", module);
            if (node->op_val == "+=") return emitCall(builder, fAdd, {lhsVal, rhsVal});
            if (node->op_val == "-=") return emitCall(builder, fSub, {lhsVal, rhsVal});
            if (node->op_val == "*=") return emitCall(builder, fMul, {lhsVal, rhsVal});
            if (node->op_val == "/=") return emitCall(builder, fDiv, {lhsVal, rhsVal});
            if (node->op_val == "%=") return emitCall(builder, fMod, {lhsVal, rhsVal});
            if (node->op_val == "&=") return emitCall(builder, fConcat, {lhsVal, rhsVal});
            return rhsVal;
        };

        if (node->left->type == ExprAST::VarIndex) {
            // Peel the full index chain of a nested target (a[1][5] = v →
            // base `a`, chain [1, 5]) into owned nodes, then assign through the
            // deep helper so missing intermediate array rows are auto-created
            // (CF: a = ArrayNew(2); a[1][5] = 1).
            std::unique_ptr<ExprAST> base = std::move(node->left);
            std::vector<std::unique_ptr<ExprAST>> idxASTs;
            while (base->type == ExprAST::VarIndex) {
                idxASTs.push_back(std::move(base->right));
                base = std::move(base->left);
            }
            std::reverse(idxASTs.begin(), idxASTs.end());
            auto *arr = CompileExprAST(module, builder, function, base, cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *rhs = CompileExprAST(module, builder, function, node->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
            if (idxASTs.size() == 1) {
                auto *idx = CompileExprAST(module, builder, function, idxASTs[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
                auto *f = module->getFunction("cfvariant_index_assign");
                if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index_assign", module);
                if (node->op_val == "=") {
                    return emitCall(builder, f, {arr, idx, rhs});
                }
                auto *fIdx = module->getFunction("cfvariant_index");
                if (!fIdx) fIdx = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index", module);
                auto *cur = emitCall(builder, fIdx, {arr, idx});
                return emitCall(builder, f, {arr, idx, compoundOp(cur, rhs)});
            }
            auto *fDeep = module->getFunction("cfvariant_index_assign_deep");
            if (!fDeep) fDeep = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index_assign_deep", module);
            std::vector<llvm::Value*> idxVals;
            for (auto &ia : idxASTs) {
                idxVals.push_back(CompileExprAST(module, builder, function, ia, cgi, server, cookie, application, session, url, form, variables, cfm_text));
            }
            auto *idxArr = createEntryAlloca(builder, function, builder.getPtrTy(), builder.getInt32(static_cast<int>(idxVals.size())));
            for (size_t k = 0; k < idxVals.size(); k++) {
                builder.CreateStore(idxVals[k], builder.CreateGEP(builder.getPtrTy(), idxArr, builder.getInt32(static_cast<int>(k))));
            }
            if (node->op_val != "=") {
                auto *fIdx = module->getFunction("cfvariant_index");
                if (!fIdx) fIdx = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index", module);
                llvm::Value *curVal = arr;
                for (size_t k = 0; k < idxVals.size(); k++) {
                    curVal = emitCall(builder, fIdx, {curVal, idxVals[k]});
                }
                auto *fAdd = module->getFunction("cfvariant_add");
                if (!fAdd) fAdd = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_add", module);
                rhs = emitCall(builder, fAdd, {curVal, rhs});
            }
            return emitCall(builder, fDeep, {arr, idxArr, builder.getInt32(static_cast<int>(idxVals.size())), rhs});
        }
        else if (node->left->type == ExprAST::BinaryOp && node->left->op_val == ".") {
            auto *arr = CompileExprAST(module, builder, function, node->left->left, cgi, server, cookie, application, session, url, form, variables, cfm_text);
            // A glued dotted member name in the assignment target (e.g.
            // s1[key].k2.k3 = v) must walk the intermediate hops with
            // cfvariant_index and only index_assign the final one (see
            // splitMemberPath).
            if (node->left->right->type == ExprAST::Variable &&
                node->left->right->string_val.find('.') != std::string::npos) {
                std::vector<std::string> parts = splitMemberPath(node->left->right->string_val);
                std::vector<std::string> head(parts.begin(), parts.end() - 1);
                llvm::Value *base = head.empty() ? arr : emitMemberWalk(module, builder, arr, head);
                const std::string &lastSeg = parts.back();
                auto *fStr = module->getFunction("cfvariant_create_string");
                if (!fStr) fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                auto *keyVal = emitCall(builder, fStr, {getPtrGlobalString(lastSeg)});
                auto *rhs2 = CompileExprAST(module, builder, function, node->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
                auto *fAssign = module->getFunction("cfvariant_index_assign");
                if (!fAssign) fAssign = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index_assign", module);
                if (node->op_val == "=") {
                    return emitCall(builder, fAssign, {base, keyVal, rhs2});
                }
                auto *fIdx = module->getFunction("cfvariant_index");
                if (!fIdx) fIdx = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index", module);
                auto *cur = emitCall(builder, fIdx, {base, keyVal});
                return emitCall(builder, fAssign, {base, keyVal, compoundOp(cur, rhs2)});
            }
            llvm::Value *keyVal = nullptr;
            if (node->left->right->type == ExprAST::Variable) {
                auto *fStr = module->getFunction("cfvariant_create_string");
                if (!fStr) fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                keyVal = emitCall(builder, fStr, {getPtrGlobalString(node->left->right->string_val)});
            } else {
                keyVal = CompileExprAST(module, builder, function, node->left->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *rhs = CompileExprAST(module, builder, function, node->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *f = module->getFunction("cfvariant_index_assign");
            if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index_assign", module);
            if (node->op_val == "=") {
                return emitCall(builder, f, {arr, keyVal, rhs});
            }
            auto *fIdx = module->getFunction("cfvariant_index");
            if (!fIdx) fIdx = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index", module);
            auto *cur = emitCall(builder, fIdx, {arr, keyVal});
            return emitCall(builder, f, {arr, keyVal, compoundOp(cur, rhs)});
        }
        else if (node->left->type != ExprAST::Variable) {
            throw webstrada::exception("LHS of assignment must be a variable, array index, or struct member");
        }
        auto *rhs = CompileExprAST(module, builder, function, node->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
        auto *f = module->getFunction("cfvariant_assign");
        if (!f) {
            std::vector<llvm::Type*> p(10, builder.getPtrTy());
            f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), p, false), llvm::Function::InternalLinkage, "cfvariant_assign", module);
        }
        if (node->op_val == "=") {
            llvm::Value *args[] = {cgi, server, cookie, application, session, url, form, variables, getPtrGlobalString(node->left->string_val), rhs};
            return emitCall(builder, f, args);
        }
        auto *fGet = module->getFunction("cfvariant_get_var");
        if (!fGet) {
            std::vector<llvm::Type*> p(9, builder.getPtrTy());
            fGet = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), p, false), llvm::Function::InternalLinkage, "cfvariant_get_var", module);
        }
        llvm::Value *getArgs[] = {cgi, server, cookie, application, session, url, form, variables, getPtrGlobalString(node->left->string_val)};
        auto *cur = emitCall(builder, fGet, getArgs);
        llvm::Value *args2[] = {cgi, server, cookie, application, session, url, form, variables, getPtrGlobalString(node->left->string_val), compoundOp(cur, rhs)};
        return emitCall(builder, f, args2);
    }
    case ExprAST::Increment: {
        if (node->left->type != ExprAST::Variable) {
            throw webstrada::exception("Increment/decrement requires a variable");
        }
        auto *fGet = module->getFunction("cfvariant_get_var");
        if (!fGet) {
            std::vector<llvm::Type*> p(9, builder.getPtrTy());
            fGet = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), p, false), llvm::Function::InternalLinkage, "cfvariant_get_var", module);
        }
        llvm::Value *getArgs[] = {cgi, server, cookie, application, session, url, form, variables, getPtrGlobalString(node->left->string_val)};
        auto *curVal = emitCall(builder, fGet, getArgs);

        auto *fOne = module->getFunction("cfvariant_create_int");
        if (!fOne) fOne = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty()}, false), llvm::Function::InternalLinkage, "cfvariant_create_int", module);
        auto *one = emitCall(builder, fOne, {builder.getInt32(1)});

        auto *fAdd = module->getFunction("cfvariant_add");
        if (!fAdd) fAdd = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_add", module);
        auto *fSub = module->getFunction("cfvariant_sub");
        if (!fSub) fSub = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_sub", module);
        auto *newVal = (node->op_val == "+")
            ? emitCall(builder, fAdd, {curVal, one})
            : emitCall(builder, fSub, {curVal, one});

        // For post-increment (x++) the expression value is the OLD value.
        // cfvariant_get_var returns a pointer into the scope slot, and
        // cfvariant_assign writes the new value into that same slot, so a
        // post-increment must copy the old value before assigning.
        llvm::Value *oldValCopy = nullptr;
        if (!node->isPre) {
            auto *fCopy = module->getFunction("cfvariant_copy_value");
            if (!fCopy) fCopy = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_copy_value", module);
            oldValCopy = emitCall(builder, fCopy, {curVal});
        }

        auto *fAssign = module->getFunction("cfvariant_assign");
        if (!fAssign) {
            std::vector<llvm::Type*> p(10, builder.getPtrTy());
            fAssign = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), p, false), llvm::Function::InternalLinkage, "cfvariant_assign", module);
        }
        llvm::Value *assignArgs[] = {cgi, server, cookie, application, session, url, form, variables, getPtrGlobalString(node->left->string_val), newVal};
        emitCall(builder, fAssign, assignArgs);
        return node->isPre ? newVal : oldValCopy;
    }
    case ExprAST::VarIndex: {
        auto *arr = CompileExprAST(module, builder, function, node->left, cgi, server, cookie, application, session, url, form, variables, cfm_text);
        auto *idx = CompileExprAST(module, builder, function, node->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
        // A single index on a simple named variable reports CF's named
        // out-of-bounds message (killArray[1]); a chained or other base uses the
        // array-object form (arr2[3][1]). Was BUGS.md "Array index out-of-bounds
        // error message".
        if (node->left->type == ExprAST::Variable && !node->isChainedMember) {
            auto *f = module->getFunction("cfvariant_index_named");
            if (!f) {
                std::vector<llvm::Type*> p(4, builder.getPtrTy());
                p[3] = builder.getInt32Ty();
                f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), p, false), llvm::Function::InternalLinkage, "cfvariant_index_named", module);
            }
            return emitCall(builder, f, {arr, idx, getPtrGlobalString(node->left->string_val), builder.getInt32(1)});
        }
        auto *f = module->getFunction("cfvariant_index");
        if (!f) f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index", module);
        return emitCall(builder, f, {arr, idx});
    }
    case ExprAST::Closure: {
        // Anonymous function: compile a fresh JIT function for the body and
        // produce a callable Function value bound to the current scope.
        UdfDef def;
        def.name = "";
        def.paramNames = node->closureParams;
        def.paramTypes = node->closureParamTypes;
        def.paramDefaults = node->closureParamDefaults;
        def.bodyTokens = node->closureBody;
        def.isClosure = true;
        // CF names anonymous closures "_CF_ANONYMOUSCLOSURE_<n>" in the call
        // stack (the number is a per-JVM counter that cannot match CF's, so the
        // shape matches but not the exact index).
        std::string closureStackName = "_CF_ANONYMOUSCLOSURE_" + std::to_string(g_closureCounter);
        llvm::Function *closureFn = compileUdfFunction(module, module->getContext(), builder,
                                                       "closure_" + std::to_string(g_closureCounter++),
                                                       def, cfm_text, /*cfm_text_size*/ 0,
                                                       /*isComponentMethod*/ false,
                                                       closureStackName.c_str());
        auto *fCreateUdf = getOrCreateHelper(module, builder, "cfvariant_create_udf", builder.getPtrTy(),
                                             {builder.getPtrTy(), builder.getPtrTy(), builder.getInt1Ty(), builder.getPtrTy(), builder.getPtrTy()});
        std::string metaBlob = buildUdfMetaBlob(def, cfm_text);
        return emitCall(builder, fCreateUdf, {
            builder.CreateGlobalString("", "", 0, module, true),
            closureFn, builder.getInt1(true), variables,
            builder.CreateGlobalString(llvm::StringRef(metaBlob.data(), metaBlob.size()), "", 0, module, true)});
    }
    case ExprAST::NewExpr: {
        // cfscript `new path.Component(args)`: load, instantiate and auto-call
        // init(args) at runtime (cf_component_new).
        std::vector<llvm::Value*> compiledArgs;
        for (const auto &arg : node->args) {
            compiledArgs.push_back(CompileExprAST(module, builder, function, arg, cgi, server, cookie, application, session, url, form, variables, cfm_text));
        }
        llvm::Value *argArray = llvm::ConstantPointerNull::get(builder.getPtrTy());
        if (!compiledArgs.empty()) {
            argArray = createEntryAlloca(builder, function, builder.getPtrTy(), builder.getInt32(static_cast<int>(compiledArgs.size())));
            for (size_t i = 0; i < compiledArgs.size(); i++) {
                auto *ptr = builder.CreateGEP(builder.getPtrTy(), argArray, builder.getInt32(static_cast<int>(i)));
                builder.CreateStore(compiledArgs[i], ptr);
            }
        }
        auto *fNew = getOrCreateHelper(module, builder, "cf_component_new", builder.getPtrTy(),
                                       {builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty(),
                                        builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                        builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                        builder.getPtrTy()});
        llvm::Value *outVal = currentOut(function);
        return emitCall(builder, fNew, {builder.CreateGlobalString(node->op_val, "", 0, module, true),
                                         argArray, builder.getInt32(static_cast<int>(compiledArgs.size())),
                                         outVal, cgi, server, cookie, application, session, url, form, variables});
    }
    case ExprAST::MemberCall: {
        // base.method(args): resolve base and dispatch the member method at
        // runtime (cfvariant_member_method). This resolves a stored callable
        // (UDF/closure) in a struct/xml member first, then the built-in
        // member-method table, so both `s.hello()` (stored UDF) and
        // `s.k[2].toUpperCase()` (built-in string method) work.
        auto *base = CompileExprAST(module, builder, function, node->left, cgi, server, cookie, application, session, url, form, variables, cfm_text);
        const auto *callNode = node->right.get();
        if (!callNode || callNode->type != ExprAST::FuncCall) {
            throw webstrada::exception("Invalid member function call");
        }
        std::vector<llvm::Value*> compiledArgs;
        // Named arguments on a member call (base.method(name=value)) build the
        // marker struct passed as args[0]; the runtime member dispatch reorders
        // against the method's parameter names.
        {
            bool hasNamedArg = false;
            for (const auto &arg : callNode->args) {
                if (arg->type == ExprAST::NamedArg) { hasNamedArg = true; break; }
            }
            if (hasNamedArg) {
                auto *fSt = module->getFunction("cfvariant_create_struct");
                if (!fSt) fSt = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {}, false), llvm::Function::InternalLinkage, "cfvariant_create_struct", module);
                llvm::Value *namedStruct = emitCall(builder, fSt, {});
                auto *fSet = module->getFunction("cfvariant_index_assign");
                if (!fSet) fSet = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index_assign", module);
                auto *fStr = module->getFunction("cfvariant_create_string");
                if (!fStr) fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                for (const auto &arg : callNode->args) {
                    if (arg->type == ExprAST::NamedArg) {
                        llvm::Value *valVal = CompileExprAST(module, builder, function, arg->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
                        llvm::Value *keyVal = emitCall(builder, fStr, {getPtrGlobalString(arg->string_val)});
                        emitCall(builder, fSet, {namedStruct, keyVal, valVal});
                    } else {
                        compiledArgs.push_back(CompileExprAST(module, builder, function, arg, cgi, server, cookie, application, session, url, form, variables, cfm_text));
                    }
                }
                auto *fMarker = module->getFunction("cf_named_args_marker");
                if (!fMarker) fMarker = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cf_named_args_marker", module);
                llvm::Value *namedMarker = emitCall(builder, fMarker, {namedStruct});
                compiledArgs.insert(compiledArgs.begin(), namedMarker);
            } else {
                for (const auto &arg : callNode->args) {
                    compiledArgs.push_back(CompileExprAST(module, builder, function, arg, cgi, server, cookie, application, session, url, form, variables, cfm_text));
                }
            }
        }
        llvm::Value *argArray = llvm::ConstantPointerNull::get(builder.getPtrTy());
        if (!compiledArgs.empty()) {
            argArray = createEntryAlloca(builder, function, builder.getPtrTy(), builder.getInt32(static_cast<int>(compiledArgs.size())));
            for (size_t i = 0; i < compiledArgs.size(); i++) {
                auto *ptr = builder.CreateGEP(builder.getPtrTy(), argArray, builder.getInt32(static_cast<int>(i)));
                builder.CreateStore(compiledArgs[i], ptr);
            }
        }
        auto *fMethod = getOrCreateHelper(module, builder, "cfvariant_member_method", builder.getPtrTy(),
                                          {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty(),
                                           builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                           builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                           builder.getPtrTy()});
        llvm::Value *outVal = currentOut(function);
        return emitCall(builder, fMethod, {base, builder.CreateGlobalString(callNode->op_val, "", 0, module, true),
                                             argArray, builder.getInt32(static_cast<int>(compiledArgs.size())),
                                             outVal, cgi, server, cookie, application, session, url, form, variables});
    }
    case ExprAST::FuncCall: {
        std::string fname = node->op_val;
        for (auto &c : fname) c = toupper(c);
        // Unimplemented function JIT stubs interception
        if (fname == "PRECISIONEVALUATE") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fPe = getOrCreateHelper(module, builder, "cf_precisionevaluate", builder.getPtrTy(),
                {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                 builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                 builder.getPtrTy()});
            return emitCall(builder, fPe, {argVal, cgi, server, cookie, application, session, url, form, variables});
        }

        // ---- Cache family (sqlite-backed): CacheGet/CachePut/CacheGetAllIds/
        // CacheGetMetadata/CacheGetProperties/CacheIdExists/CacheRemove/
        // CacheRemoveAll/CacheRegionExists/CacheRegionNew/CacheRegionRemove/
        // CacheSetProperties/CacheGetSession/RemoveCachedQuery. Each is a direct
        // call to its cf_* function with the evaluated arguments (missing args
        // pass null). CACHEGETSESSION is not supported (returns a Java object).
        if (fname == "CACHEGET" || fname == "CACHEGETALLIDS" || fname == "CACHEGETMETADATA" ||
            fname == "CACHEGETPROPERTIES" || fname == "CACHEGETSESSION" || fname == "CACHEIDEXISTS" ||
            fname == "CACHEPUT" || fname == "CACHEREGIONEXISTS" || fname == "CACHEREGIONNEW" ||
            fname == "CACHEREGIONREMOVE" || fname == "CACHEREMOVE" || fname == "CACHEREMOVEALL" ||
            fname == "CACHESETPROPERTIES" || fname == "REMOVECACHEDQUERY" ||
            fname == "GETBASETAGDATA" || fname == "GETBASETAGLIST") {
            // Argument-count limits per function: (min, max).
            int minArgs = 0, maxArgs = 0;
            if (fname == "CACHEGET" || fname == "CACHEIDEXISTS") { minArgs = 1; maxArgs = 2; }
            else if (fname == "CACHEGETALLIDS" || fname == "CACHEGETPROPERTIES" || fname == "CACHEREMOVEALL") { minArgs = 0; maxArgs = 2; }
            else if (fname == "CACHEGETMETADATA") { minArgs = 1; maxArgs = 3; }
            else if (fname == "CACHEGETSESSION") { minArgs = 1; maxArgs = 2; }
            else if (fname == "CACHEPUT") { minArgs = 2; maxArgs = 6; }
            else if (fname == "CACHEREGIONEXISTS") { minArgs = 1; maxArgs = 1; }
            else if (fname == "CACHEREGIONNEW") { minArgs = 1; maxArgs = 3; }
            else if (fname == "CACHEREGIONREMOVE") { minArgs = 1; maxArgs = 1; }
            else if (fname == "CACHEREMOVE") { minArgs = 1; maxArgs = 4; }
            else if (fname == "CACHESETPROPERTIES") { minArgs = 1; maxArgs = 2; }
            else if (fname == "REMOVECACHEDQUERY") { minArgs = 1; maxArgs = 4; }
            else if (fname == "GETBASETAGLIST") { minArgs = 0; maxArgs = 0; }
            else if (fname == "GETBASETAGDATA") { minArgs = 1; maxArgs = 2; }
            if ((int)node->args.size() < minArgs || (int)node->args.size() > maxArgs) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires " +
                    std::to_string(minArgs) + " to " + std::to_string(maxArgs) + " arguments").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            std::vector<llvm::Type*> paramTypes(maxArgs, builder.getPtrTy());
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), paramTypes, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            std::vector<llvm::Value*> callArgs;
            for (int i = 0; i < maxArgs; i++) {
                if (i < (int)node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            return emitCall(builder, fHelper, callArgs);
        }

        if (fname == "ADDSOAPREQUESTHEADER" || fname == "ADDSOAPRESPONSEHEADER" || fname == "AUTHENTICATEDCONTEXT" || fname == "AUTHENTICATEDUSER" ||             fname == "CREATEENCRYPTEDJWT" || fname == "CREATESIGNEDJWT" || 
            fname == "DOTNETTOCFTYPE" || fname == "ENTITYDELETE" || fname == "ENTITYLOAD" ||
            fname == "ENTITYLOADBYEXAMPLE" || fname == "ENTITYLOADBYPK" || fname == "ENTITYMERGE" || fname == "ENTITYNEW" || fname == "ENTITYRELOAD" || fname == "ENTITYSAVE" || fname == "ENTITYTOQUERY" || fname == "GENERATESAMLSPMETADATA" ||
            fname == "GETFUNCTIONCALLEDNAME" || fname == "GETGATEWAYHELPER" || fname == "GETK2SERVERDOCCOUNT" || fname == "GETK2SERVERDOCCOUNTLIMIT" || fname == "GETPAGECONTEXT" || fname == "GETPRINTERINFO" || fname == "GETPRINTERLIST" ||
            fname == "GETSAFEHTML" || fname == "GETSAMLAUTHREQUEST" || fname == "GETSAMLLOGOUTREQUEST" || fname == "GETSOAPREQUEST" || fname == "GETSOAPREQUESTHEADER" || fname == "GETSOAPRESPONSE" || fname == "GETSOAPRESPONSEHEADER" ||             fname == "GETVFSMETADATA"  || fname == "HQLMETHODS" ||
            fname == "INITSAMLAUTHREQUEST" || fname == "INITSAMLLOGOUTREQUEST" || fname == "INTERRUPTTHREAD" || fname == "INVALIDATEOAUTHACCESSTOKEN" || fname == "ISAUTHENTICATED" || fname == "ISAUTHORIZED" ||
            fname == "ISK2SERVERABROKER" || fname == "ISK2SERVERDOCCOUNTEXCEEDED" || fname == "ISK2SERVERONLINE" || fname == "ISPROTECTED" || fname == "ISSAFEHTML" || fname == "ISSAMLLOGOUTRESPONSE" || fname == "ISSOAPREQUEST" ||
            fname == "ISSPREADSHEETFILE" || fname == "ISSPREADSHEETOBJECT" || fname == "ISVALIDOAUTHACCESSTOKEN" || fname == "JAVACAST" ||
            fname == "NUMBERFORMAT" || fname == "ONWSAUTHENTICATE" || fname == "ORMCLEARSESSION" || fname == "ORMCLOSEALLSESSIONS" || fname == "ORMCLOSESESSION" || fname == "ORMEVICTCOLLECTION" || fname == "ORMEVICTENTITY" || fname == "ORMEVICTQUERIES" || fname == "ORMEXECUTEQUERY" || fname == "ORMFLUSH" || fname == "ORMFLUSHALL" || fname == "ORMGETSESSION" || fname == "ORMGETSESSIONFACTORY" ||
            fname == "ORMINDEX" || fname == "ORMINDEXPURGE" || fname == "ORMRELOAD" || fname == "ORMSEARCH" || fname == "ORMSEARCHOFFLINE" || fname == "PROCESSSAMLLOGOUTREQUEST" || fname == "PROCESSSAMLRESPONSE" ||
            fname == "RELEASECOMOBJECT" || fname == "RESTDELETEAPPLICATION" || fname == "RESTINITAPPLICATION" || fname == "RESTSETRESPONSE" || fname == "SENDGATEWAYMESSAGE" || fname == "SENDSAMLLOGOUTRESPONSE" ||             fname == "SETENCODING" ||
            fname == "SPREADSHEETADDAUTOFILTER" || fname == "SPREADSHEETADDCOLUMN" || fname == "SPREADSHEETADDFREEZEPANE" || fname == "SPREADSHEETADDIMAGE" || fname == "SPREADSHEETADDINFO" || fname == "SPREADSHEETADDPAGEBREAKS" || fname == "SPREADSHEETADDPRINTGRIDLINES" || fname == "SPREADSHEETADDROW" || fname == "SPREADSHEETADDROWS" || fname == "SPREADSHEETADDSPLITPANE" || fname == "SPREADSHEETCREATESHEET" || fname == "SPREADSHEETDELETECOLUMN" || fname == "SPREADSHEETDELETECOLUMNS" || fname == "SPREADSHEETDELETEROW" || fname == "SPREADSHEETDELETEROWS" || fname == "SPREADSHEETFORMATCELL" || fname == "SPREADSHEETFORMATCELLRANGE" || fname == "SPREADSHEETFORMATCOLUMN" || fname == "SPREADSHEETFORMATCOLUMNS" || fname == "SPREADSHEETFORMATROW" || fname == "SPREADSHEETFORMATROWS" || fname == "SPREADSHEETGETCELLCOMMENT" || fname == "SPREADSHEETGETCELLFORMULA" || fname == "SPREADSHEETGETCELLVALUE" || fname == "SPREADSHEETGETCOLUMNCOUNT" || fname == "SPREADSHEETGETCOLUMNWIDTH" || fname == "SPREADSHEETGETLASTROWNUMBER" || fname == "SPREADSHEETGETPRINTORIENTATION" ||
            fname == "SPREADSHEETGROUPCOLUMNS" || fname == "SPREADSHEETGROUPROWS" || fname == "SPREADSHEETINFO" || fname == "SPREADSHEETISBINARYFORMAT" || fname == "SPREADSHEETISCOLUMNHIDDEN" || fname == "SPREADSHEETISROWHIDDEN" || fname == "SPREADSHEETISSTREAMINGXMLFORMAT" || fname == "SPREADSHEETISXMLFORMAT" || fname == "SPREADSHEETMERGECELLS" || fname == "SPREADSHEETNEW" || fname == "SPREADSHEETREAD" || fname == "SPREADSHEETREADBINARY" || fname == "SPREADSHEETREMOVECOLUMNBREAK" || fname == "SPREADSHEETREMOVEPRINTGRIDLINES" || fname == "SPREADSHEETREMOVEROWBREAK" || fname == "SPREADSHEETREMOVESHEET" || fname == "SPREADSHEETREMOVESHEETNUMBER" || fname == "SPREADSHEETRENAMESHEET" || fname == "SPREADSHEETSETACTIVESHEET" || fname == "SPREADSHEETSETACTIVESHEETNUMBER" || fname == "SPREADSHEETSETCELLCOMMENT" || fname == "SPREADSHEETSETCELLFORMULA" || fname == "SPREADSHEETSETCELLVALUE" || fname == "SPREADSHEETSETCOLUMNBREAK" || fname == "SPREADSHEETSETCOLUMNHIDDEN" || fname == "SPREADSHEETSETCOLUMNWIDTH" || fname == "SPREADSHEETSETFITTOPAGE" || fname == "SPREADSHEETSETFOOTER" || fname == "SPREADSHEETSETFOOTERIMAGE" || fname == "SPREADSHEETSETHEADER" ||
             fname == "SPREADSHEETSETHEADERIMAGE" || fname == "SPREADSHEETSETROWBREAK" || fname == "SPREADSHEETSETROWHEIGHT" || fname == "SPREADSHEETSETROWHIDDEN" || fname == "SPREADSHEETSHIFTCOLUMNS" || fname == "SPREADSHEETSHIFTROWS" || fname == "SPREADSHEETUNGROUPCOLUMNS" || fname == "SPREADSHEETUNGROUPROWS" || fname == "SPREADSHEETWRITE" || fname == "STOREADDACL" || fname == "STOREGETACL" || fname == "STOREGETMETADATA" || fname == "STORESETACL" || fname == "STORESETMETADATA" || fname == "STREAMINGSPREADSHEETCLEANUP" || fname == "STREAMINGSPREADSHEETNEW" || fname == "STREAMINGSPREADSHEETPROCESS" || fname == "STREAMINGSPREADSHEETREAD" || fname == "STREAMINGSPREADSHEETISSTREAMINGXMLFORMAT" || fname == "STREAMINGSPREADSHEETISXMLFORMAT" ||
             fname == "THREADJOIN" || fname == "THREADTERMINATE" || fname == "THROW" || fname == "VERIFYCLIENT" || fname == "WSGETALLCHANNELS" || fname == "WSGETSUBSCRIBERS" || fname == "WSPUBLISH" || fname == "WSSENDMESSAGE") {
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {});
        }

        // Auth functions (cflogin model). GetAuthUser()/GetUserRoles()/
        // IsUserLoggedIn() take no arguments; IsUserInRole()/IsUserInAnyRole()
        // take one. The runtime reads the per-request security context.
        if (fname == "GETAUTHUSER" || fname == "GETUSERROLES" || fname == "ISUSERLOGGEDIN") {
            if (!node->args.empty()) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " does not take any arguments").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {});
        }

        if (fname == "ISUSERINROLE" || fname == "ISUSERINANYROLE") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }


        // as an array of {Template, LineNumber, Function} structs.
        if (fname == "CALLSTACKGET") {
            if (!node->args.empty()) {
                throw webstrada::exception("CallStackGet requires 0 arguments");
            }
            auto *fHelper = module->getFunction("cf_callstackget");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {}, false),
                    llvm::Function::InternalLinkage, "cf_callstackget", module
                );
            }
            return emitCall(builder, fHelper, {});
        }

        // CallStackDump([output]): writes the call stack text to the page
        // output (default / "browser") or to the file at `output`. Returns a
        // discarded value.
        if (fname == "CALLSTACKDUMP") {
            if (node->args.size() > 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 0 to 1 arguments").c_str()));
            }
            llvm::Value *outputArg = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (!node->args.empty()) {
                outputArg = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *fHelper = module->getFunction("cf_callstackdump");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_callstackdump", module
                );
            }
            return emitCall(builder, fHelper, {currentOut(function), outputArg});
        }

        // Native JIT compilation handler for WriteDump: writedump(var [, output]
        // [, format] [, abort] [, label] [, metainfo] [, top] [, show] [, hide]
        // [, keys] [, expand] [, showUDFs]). The dump string is emitted to the
        // page output immediately because cfscript statements discard the
        // expression result.
        if (fname == "WRITEDUMP") {
            if (node->args.size() < 1 || node->args.size() > 12) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 to 12 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 12; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_writedump");
            if (!fHelper) {
                std::vector<llvm::Type*> params(12, builder.getPtrTy());
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), params, false),
                    llvm::Function::InternalLinkage, "cf_writedump", module
                );
            }
            auto *dumpResult = emitCall(builder, fHelper, callArgs);
            auto *fEmit = module->getFunction("cf_emit_writedump");
            if (!fEmit) {
                fEmit = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getVoidTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_emit_writedump", module
                );
            }
            emitCall(builder, fEmit, {currentOut(function), dumpResult});
            return dumpResult;
        }

        // Native JIT compilation handler for WriteLog: writelog(text [, type]
        // [, application] [, file] [, log]). Named arguments are bound by
        // parameter name (case-insensitive) and reordered like CF's argument
        // binding; positional arguments fill the remaining slots in order. The
        // return value is discarded.
        if (fname == "WRITELOG") {
            if (node->args.size() < 1 || node->args.size() > 5) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 to 5 arguments").c_str()));
            }
            static const char *logParams[] = {"text", "type", "application", "file", "log"};
            std::vector<int> slotArgIdx(5, -1);
            int positional = 0;
            for (size_t ai = 0; ai < node->args.size(); ai++) {
                const auto &arg = node->args[ai];
                if (arg->type == ExprAST::NamedArg) {
                    std::string low;
                    for (char c : arg->string_val) low.push_back((char)tolower((unsigned char)c));
                    for (int p = 0; p < 5; p++) {
                        if (low == logParams[p]) { slotArgIdx[p] = (int)ai; break; }
                    }
                } else if (positional < 5) {
                    slotArgIdx[positional] = (int)ai;
                    positional++;
                }
            }
            std::vector<llvm::Value*> callArgs;
            llvm::Value *nullPtr = llvm::ConstantPointerNull::get(builder.getPtrTy());
            for (int i = 0; i < 5; i++) {
                if (slotArgIdx[i] >= 0) {
                    const auto &arg = node->args[slotArgIdx[i]];
                    if (arg->type == ExprAST::NamedArg && arg->right) {
                        callArgs.push_back(CompileExprAST(module, builder, function, arg->right, cgi, server, cookie, application, session, url, form, variables, cfm_text));
                    } else {
                        callArgs.push_back(CompileExprAST(module, builder, function, arg, cgi, server, cookie, application, session, url, form, variables, cfm_text));
                    }
                } else {
                    callArgs.push_back(nullPtr);
                }
            }
            auto *fHelper = module->getFunction("cf_writelog");
            if (!fHelper) {
                std::vector<llvm::Type*> params(5, builder.getPtrTy());
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), params, false),
                    llvm::Function::InternalLinkage, "cf_writelog", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // Native JIT compilation handler for IsDefined: isDefined(variable_name)
        // evaluates the string argument against the runtime scope maps (CGI,
        // SERVER, COOKIE, APPLICATION, SESSION, URL, FORM, VARIABLES) and returns
        // a boolean. It needs the live scope pointers, so it is a direct call
        // (no dynamic lookup), passing the 8 scopes plus the name.
        if (fname == "ISDEFINED") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_isdefined");
            if (!fHelper) {
                std::vector<llvm::Type*> params(9, builder.getPtrTy());
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), params, false),
                    llvm::Function::InternalLinkage, "cf_isdefined", module
                );
            }
            llvm::Value *args[] = {argVal, cgi, server, cookie, application, session, url, form, variables};
            return emitCall(builder, fHelper, args);
        }

        if (fname == "GETHTTPREQUESTDATA") {
            // getHttpRequestData([includeBody]): returns a struct with the
            // request headers/method/protocol and (when includeBody, default
            // true) the raw request body. Needs the CGI scope.
            if (node->args.size() > 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 0 or 1 arguments").c_str()));
            }
            llvm::Value *includeBody = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (node->args.size() == 1) {
                includeBody = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *fHelper = module->getFunction("cf_gethttprequestdata");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_gethttprequestdata", module
                );
            }
            return emitCall(builder, fHelper, {cgi, includeBody});
        }

        // Native JIT compilation handler for Evaluate: evaluate(expr1, expr2,
        // ...) dynamically evaluates the string value of each argument (left to
        // right) and returns the rightmost result. It needs the live scope
        // pointers and the page output buffer, so it is a direct call that
        // passes the compiled argument values plus the 8 scopes and out.
        if (fname == "EVALUATE") {
            if (node->args.size() < 1) {
                throw webstrada::exception("Parameter validation error for the EVALUATE function: The function takes 1 or more parameters.");
            }
        std::vector<llvm::Value*> compiledArgs;
        for (const auto &arg : node->args) {
            compiledArgs.push_back(CompileExprAST(module, builder, function, arg, cgi, server, cookie, application, session, url, form, variables, cfm_text));
        }
            llvm::Value *argArray = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (!compiledArgs.empty()) {
                argArray = createEntryAlloca(builder, function, builder.getPtrTy(), builder.getInt32(static_cast<int>(compiledArgs.size())));
                for (size_t i = 0; i < compiledArgs.size(); i++) {
                    auto *ptr = builder.CreateGEP(builder.getPtrTy(), argArray, builder.getInt32(static_cast<int>(i)));
                    builder.CreateStore(compiledArgs[i], ptr);
                }
            }
            auto *fEval = getOrCreateHelper(module, builder, "cf_evaluate", builder.getPtrTy(),
                                            {builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty(),
                                             builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                             builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                                             builder.getPtrTy()});
            llvm::Value *outVal = currentOut(function);
            return emitCall(builder, fEval, {outVal, argArray, builder.getInt32(static_cast<int>(compiledArgs.size())),
                                             cgi, server, cookie, application, session, url, form, variables});
        }

        // Native JIT compilation handlers for XML functions
        if (fname == "ISXML" || fname == "ISXMLATTRIBUTE" || fname == "ISXMLDOC" || fname == "ISXMLELEM" ||
            fname == "ISXMLNODE" || fname == "ISXMLROOT" || fname == "SERIALIZEXML" || fname == "XMLGETNODETYPE") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);

            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        if (fname == "XMLNEW" || fname == "GETHTTPTIMESTRING") {
            if (node->args.size() > 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires at most 1 argument").c_str()));
            }
            llvm::Value *arg1 = nullptr;
            if (node->args.size() == 1) {
                arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg1 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {arg1});
        }

        if (fname == "QUERYNEW") {
            if (node->args.size() < 1 || node->args.size() > 3) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 to 3 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *arg2 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (node->args.size() >= 2) {
                arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            llvm::Value *arg3 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (node->args.size() == 3) {
                arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *fHelper = module->getFunction("cf_querynew");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_querynew", module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2, arg3});
        }

        if (fname == "XMLFORMAT" || fname == "XMLVALIDATE") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 or 2 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *arg2 = nullptr;
            if (node->args.size() == 2) {
                arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg2 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2});
        }

        if (fname == "XMLPARSE" || fname == "DESERIALIZEXML") {
            if (node->args.size() < 1 || node->args.size() > 3) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires between 1 and 3 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *arg2 = nullptr;
            if (node->args.size() >= 2) {
                arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg2 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            llvm::Value *arg3 = nullptr;
            if (node->args.size() == 3) {
                arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg3 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2, arg3});
        }

        if (fname == "XMLELEMNEW" || fname == "XMLSEARCH" || fname == "XMLTRANSFORM") {
            if (node->args.size() < 2 || node->args.size() > 3) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 2 or 3 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *arg3 = nullptr;
            if (node->args.size() == 3) {
                arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg3 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2, arg3});
        }

        if (fname == "XMLCHILDPOS") {
            if (node->args.size() != 3) {
                throw webstrada::exception("XmlChildPos requires exactly 3 arguments");
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_xmlchildpos");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    "cf_xmlchildpos",
                    module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2, arg3});
        }

        if (fname == "ABS") {
            if (node->args.size() != 1) {
                throw webstrada::exception("Abs requires exactly 1 argument");
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fAbs = module->getFunction("cf_abs");
            if (!fAbs) {
                fAbs = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    "cf_abs",
                    module
                );
            }
            return emitCall(builder, fAbs, {argVal});
        }

        if (fname == "ASC" || fname == "CHR" || fname == "ACOS" || fname == "ASIN" || fname == "ATAN" ||
            fname == "CEILING" || fname == "COS" || fname == "EXP" || fname == "FLOOR" ||
            fname == "INCREMENTVALUE" || fname == "DECREMENTVALUE" || fname == "INT" ||
            fname == "LOG" || fname == "LOG10" || fname == "ROUND" || fname == "SGN" ||
            fname == "SIN" || fname == "SQR" || fname == "TAN" ||
            fname == "LEN" || fname == "TRIM" || fname == "LTRIM" || fname == "RTRIM" ||
            fname == "LCASE" || fname == "UCASE" || fname == "REVERSE" ||
            fname == "DECIMALFORMAT" || fname == "DOLLARFORMAT" || fname == "YESNOFORMAT" ||
            fname == "ISDATE" || fname == "ISQUERY" || fname == "YEAR" || fname == "MONTH" || fname == "DAY" ||
            fname == "HOUR" || fname == "MINUTE" || fname == "SECOND" ||
            fname == "DAYOFWEEK" || fname == "DAYOFYEAR" || fname == "DAYSINMONTH" || fname == "DAYSINYEAR" ||
            fname == "FIRSTDAYOFMONTH" || fname == "ISDATEOBJECT" || fname == "ISLEAPYEAR" || fname == "ISNUMERICDATE" ||
            fname == "LSISDATE" || fname == "QUARTER" || fname == "WEEK" ||
            fname == "ISBINARY" || fname == "ISBOOLEAN" || fname == "ISCLOSURE" || fname == "ISCUSTOMFUNCTION" ||
            fname == "ISFILEOBJECT" || fname == "ISIMAGE" || fname == "ISNULL" || fname == "ISNUMERIC" ||
            fname == "LSISNUMERIC" || fname == "ISOBJECT" || fname == "ISSIMPLEVALUE" || fname == "ISSTRUCT" ||
            fname == "FILEISEOF" || fname == "CREATEODBCDATETIME") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            std::string libName = (fname == "ATN") ? "cf_atan" : "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);

            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        if (fname == "RANDOMIZE") {
            // randomize(number [, algorithm]): 1 or 2 arguments.
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 or 2 arguments").c_str()));
            }
            std::string libName = "cf_randomize";
            auto *num = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *alg = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (node->args.size() == 2) {
                alg = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {num, alg});
        }

        if (fname == "RANDRANGE") {
            // randRange(number1, number2 [, algorithm]): 2 or 3 arguments.
            if (node->args.size() < 2 || node->args.size() > 3) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 2 or 3 arguments").c_str()));
            }
            std::string libName = "cf_randrange";
            auto *n1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *n2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *alg = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (node->args.size() == 3) {
                alg = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {n1, n2, alg});
        }

        if (fname == "ATAN2" || fname == "MAX" || fname == "MIN" ||
            fname == "LEFT" || fname == "RIGHT" || fname == "REPEATSTRING" ||
            fname == "COMPARE" || fname == "COMPARENOCASE" ||
            fname == "DATECONVERT" || fname == "DATEPART" || fname == "SETDAY" || fname == "SETHOUR" ||
            fname == "SETMINUTE" || fname == "SETMONTH" || fname == "SETSECOND" || fname == "SETYEAR" ||
            fname == "BINARYDECODE") {
            if (node->args.size() != 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 2 arguments").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);

            auto *argVal1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *argVal2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {argVal1, argVal2});
        }

        if (fname == "NOW" || fname == "PI" || fname == "GETTIMEZONEINFO") {
            if (node->args.size() > 0 && !(node->args.size() == 1 && node->args[0]->type == ExprAST::LiteralNull)) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 0 arguments").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);

            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {});
        }

        if (fname == "RAND") {
            // rand([algorithm]): 0 or 1 arguments.
            if (node->args.size() > 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 0 or 1 arguments").c_str()));
            }
            std::string libName = "cf_rand";
            llvm::Value *alg = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (node->args.size() == 1) {
                alg = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {alg});
        }

        // APPLICATION/SESSION scope functions: applicationStop(),
        // getApplicationMetadata(), sessionGetMetadata(), sessionInvalidate(),
        // sessionRotate().
        if (fname == "APPLICATIONSTOP" || fname == "GETAPPLICATIONMETADATA" ||
            fname == "SESSIONGETMETADATA" || fname == "SESSIONINVALIDATE" ||
            fname == "SESSIONROTATE") {
            if (node->args.size() > 0 && !(node->args.size() == 1 && node->args[0]->type == ExprAST::LiteralNull)) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 0 arguments").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {});
        }

        if (fname == "GETREADABLEIMAGEFORMATS" || fname == "GETWRITEABLEIMAGEFORMATS") {
            if (node->args.size() > 0 && !(node->args.size() == 1 && node->args[0]->type == ExprAST::LiteralNull)) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 0 arguments").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {});
        }

        if (fname == "IMAGEREAD" || fname == "IMAGEREADBASE64" || fname == "IMAGEGETBLOB" ||
            fname == "IMAGEGETWIDTH" || fname == "IMAGEGETHEIGHT" || fname == "IMAGEINFO" ||
            fname == "IMAGEGETMETADATA") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        if (fname == "IMAGEGETBUFFEREDIMAGE" || fname == "IMAGEGETEXIFMETADATA" ||
            fname == "IMAGEGETIPTCMETADATA") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        if (fname == "IMAGEGETEXIFTAG" || fname == "IMAGEGETIPTCTAG") {
            if (node->args.size() != 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 2 arguments").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
            }
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        if (fname == "IMAGECREATECAPTCHA") {
            if (node->args.size() < 3 || node->args.size() > 6) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 3 to 6 arguments").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 6; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                std::vector<llvm::Type*> params(6, builder.getPtrTy());
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), params, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        if (fname == "IMAGEWRITE" || fname == "IMAGEWRITEBASE64" || fname == "IMAGENEW" || fname == "ISIMAGEFILE") {
            size_t maxArgs = (fname == "IMAGEWRITE") ? 4 : ((fname == "IMAGEWRITEBASE64") ? 5 : ((fname == "ISIMAGEFILE") ? 2 : 5));
            size_t minArgs = (fname == "IMAGENEW") ? 0 : ((fname == "ISIMAGEFILE") ? 1 : 2);
            if (node->args.size() < minArgs || node->args.size() > maxArgs) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires " + std::to_string(minArgs) + " to " + std::to_string(maxArgs) + " arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < maxArgs; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                std::vector<llvm::Type*> params(maxArgs, builder.getPtrTy());
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), params, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        if (fname == "IMAGECLEARRECT" || fname == "IMAGEDRAWLINE" || fname == "IMAGEDRAWLINES" ||
            fname == "IMAGEDRAWRECT" || fname == "IMAGEDRAWROUNDRECT" || fname == "IMAGEDRAWBEVELEDRECT" ||
            fname == "IMAGEDRAWOVAL" || fname == "IMAGEDRAWARC" || fname == "IMAGEDRAWCUBICCURVE" ||
            fname == "IMAGEDRAWQUADRATICCURVE" || fname == "IMAGEDRAWPOINT" ||
            fname == "IMAGESETANTIALIASING" || fname == "IMAGESETBACKGROUNDCOLOR" ||
            fname == "IMAGESETDRAWINGCOLOR" || fname == "IMAGESETDRAWINGSTROKE" ||
            fname == "IMAGESETDRAWINGTRANSPARENCY" || fname == "IMAGEXORDRAWINGMODE" ||
            fname == "IMAGEADDBORDER" || fname == "IMAGEBLUR" || fname == "IMAGECOPY" ||
            fname == "IMAGECROP" || fname == "IMAGEFLIP" || fname == "IMAGEGRAYSCALE" ||
            fname == "IMAGEMAKECOLORTRANSPARENT" || fname == "IMAGEMAKETRANSLUCENT" ||
            fname == "IMAGENEGATIVE" || fname == "IMAGEOVERLAY" || fname == "IMAGEPASTE" ||
            fname == "IMAGERESIZE" || fname == "IMAGEROTATE" || fname == "IMAGESCALETOFIT" ||
            fname == "IMAGESHARPEN" || fname == "IMAGESHEAR" || fname == "IMAGETRANSLATE") {
            size_t maxArgs = (fname == "IMAGEDRAWCUBICCURVE") ? 9 :
                             (fname == "IMAGEDRAWARC" || fname == "IMAGEDRAWROUNDRECT") ? 8 :
                             (fname == "IMAGEDRAWBEVELEDRECT" || fname == "IMAGEDRAWQUADRATICCURVE") ? 7 :
                             (fname == "IMAGEDRAWOVAL" || fname == "IMAGEDRAWRECT") ? 6 :
                             (fname == "IMAGECLEARRECT" || fname == "IMAGEDRAWLINE" || fname == "IMAGEDRAWLINES") ? 5 :
                             (fname == "IMAGECOPY") ? 7 :
                             (fname == "IMAGERESIZE" || fname == "IMAGESCALETOFIT") ? 5 :
                             (fname == "IMAGEROTATE" || fname == "IMAGESHEAR") ? 5 :
                             (fname == "IMAGEADDBORDER") ? 4 :
                             (fname == "IMAGEOVERLAY") ? 4 :
                             (fname == "IMAGETRANSLATE") ? 4 :
                              (fname == "IMAGEPASTE" || fname == "IMAGECROP") ? 5 :
                              (fname == "IMAGEDRAWPOINT") ? 3 :
                              (fname == "IMAGEFLIP" || fname == "IMAGEBLUR" ||
                               fname == "IMAGEMAKETRANSLUCENT" || fname == "IMAGESHARPEN" ||
                               fname == "IMAGESETBACKGROUNDCOLOR" || fname == "IMAGESETDRAWINGCOLOR" ||
                               fname == "IMAGEXORDRAWINGMODE") ? 2 :
                              (fname == "IMAGEGRAYSCALE" || fname == "IMAGENEGATIVE") ? 1 : 2;
            size_t minArgs = (fname == "IMAGESETANTIALIASING" || fname == "IMAGESETDRAWINGSTROKE") ? 1 :
                             (fname == "IMAGEDRAWARC" || fname == "IMAGEDRAWROUNDRECT") ? 7 :
                             (fname == "IMAGEDRAWBEVELEDRECT") ? 6 :
                             (fname == "IMAGEDRAWOVAL" || fname == "IMAGEDRAWRECT") ? 5 :
                             (fname == "IMAGEDRAWLINES") ? 3 :
                             (fname == "IMAGEADDBORDER") ? 2 :
                             (fname == "IMAGECOPY" || fname == "IMAGECROP") ? 5 :
                             (fname == "IMAGEROTATE") ? 2 :
                             (fname == "IMAGESHEAR") ? 2 :
                             (fname == "IMAGEOVERLAY" || fname == "IMAGEPASTE") ? 4 :
                             (fname == "IMAGERESIZE") ? 3 :
                             (fname == "IMAGESCALETOFIT") ? 3 :
                             (fname == "IMAGETRANSLATE") ? 3 :
                              (fname == "IMAGEBLUR" || fname == "IMAGEFLIP" ||
                               fname == "IMAGEMAKETRANSLUCENT" || fname == "IMAGESHARPEN") ? 1 :
                              (fname == "IMAGEGRAYSCALE" || fname == "IMAGENEGATIVE") ? 1 : maxArgs;
            if (node->args.size() < minArgs || node->args.size() > maxArgs) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires " + std::to_string(minArgs) + " to " + std::to_string(maxArgs) + " arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < maxArgs; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                std::vector<llvm::Type*> params(maxArgs, builder.getPtrTy());
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), params, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        if (fname == "IMAGEDRAWTEXT" || fname == "IMAGEROTATEDRAWINGAXIS" ||
            fname == "IMAGESHEARDRAWINGAXIS" || fname == "IMAGETRANSLATEDRAWINGAXIS") {
            size_t maxArgs = (fname == "IMAGEDRAWTEXT") ? 5 :
                             (fname == "IMAGEROTATEDRAWINGAXIS") ? 4 : 3;
            size_t minArgs = (fname == "IMAGEDRAWTEXT") ? 4 :
                             (fname == "IMAGEROTATEDRAWINGAXIS") ? 2 : 3;
            if (node->args.size() < minArgs || node->args.size() > maxArgs) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires " + std::to_string(minArgs) + " to " + std::to_string(maxArgs) + " arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < maxArgs; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                std::vector<llvm::Type*> params(maxArgs, builder.getPtrTy());
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), params, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        if (fname == "DAYOFWEEKASSTRING" || fname == "MONTHASSTRING" ||
            fname == "PARSEDATETIME" || fname == "LSPARSEDATETIME") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 or 2 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *arg2 = nullptr;
            if (node->args.size() == 2) {
                arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg2 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage,
                    libName,
                    module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2});
        }

        if (fname == "MID" || fname == "DATEADD" || fname == "DATEDIFF") {
            if (node->args.size() != 3) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 3 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2, arg3});
        }

        if (fname == "DATECOMPARE" || fname == "LSDATEFORMAT" || fname == "LSDATETIMEFORMAT" || fname == "LSTIMEFORMAT") {
            size_t minArgs = (fname == "DATECOMPARE") ? 2 : 1;
            if (node->args.size() < minArgs || node->args.size() > 3) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires " + std::to_string(minArgs) + " to 3 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *arg2 = nullptr;
            if (node->args.size() >= 2) {
                arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg2 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            llvm::Value *arg3 = nullptr;
            if (node->args.size() == 3) {
                arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg3 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2, arg3});
        }

        if (fname == "CREATETIMESPAN") {
            if (node->args.size() != 4) {
                throw webstrada::exception("CreateTimeSpan requires exactly 4 arguments");
            }
            std::vector<llvm::Value*> args;
            for (int i = 0; i < 4; i++) {
                args.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
            }
            auto *fHelper = module->getFunction("cf_createtimespan");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_createtimespan", module
                );
            }
            return emitCall(builder, fHelper, args);
        }

        if (fname == "FIND" || fname == "FINDNOCASE") {
            if (node->args.size() < 2 || node->args.size() > 3) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 2 or 3 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *arg3 = nullptr;
            if (node->args.size() == 3) {
                arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg3 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            std::string libName = (fname == "FIND") ? "cf_find" : "cf_findnocase";
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2, arg3});
        }

        if (fname == "REPLACE" || fname == "REPLACENOCASE") {
            if (node->args.size() < 3 || node->args.size() > 4) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 3 or 4 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *arg4 = nullptr;
            if (node->args.size() == 4) {
                arg4 = CompileExprAST(module, builder, function, node->args[3], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg4 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            std::string libName = (fname == "REPLACE") ? "cf_replace" : "cf_replacenocase";
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2, arg3, arg4});
        }

        if (fname == "CREATEDATETIME") {
            if (node->args.size() != 6) {
                throw webstrada::exception("CreateDateTime requires exactly 6 arguments");
            }
            std::vector<llvm::Value*> args;
            for (int i = 0; i < 6; i++) {
                args.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
            }
            auto *fHelper = module->getFunction("cf_createdatetime");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_createdatetime", module
                );
            }
            return emitCall(builder, fHelper, args);
        }

        if (fname == "CREATEDATE" || fname == "CREATETIME") {
            if (node->args.size() != 3) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 3 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2, arg3});
        }

        // FileUpload: fileUpload(destination [, fileField] [, mimeType] [, onConflict] [, strict])
        if (fname == "FILEUPLOAD") {
            if (node->args.size() < 1 || node->args.size() > 5) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 to 5 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 5; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_fileupload");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_fileupload", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // FileUploadAll: fileUploadAll(destination [,mimeType] [,onConflict] [,strict] [,continueOnError] [,errorVariable] [,allowedExtensions])
        if (fname == "FILEUPLOADALL") {
            if (node->args.size() < 1 || node->args.size() > 7) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 to 7 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            // The runtime function needs the variables scope to store upload errors.
            callArgs.push_back(variables);
            for (size_t i = 0; i < 7; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_fileuploadall");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_fileuploadall", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // Hash: hash(string [, algorithm] [, encoding] [, additionalIterations])
        if (fname == "HASH") {
            if (node->args.size() < 1 || node->args.size() > 4) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 to 4 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 4; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_hash");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_hash", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // HMac: hmac(message, key [, algorithm] [, encoding])
        if (fname == "HMAC") {
            if (node->args.size() < 2 || node->args.size() > 4) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 2 to 4 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 4; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_hmac");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_hmac", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // Encrypt/Decrypt/EncryptBinary/DecryptBinary: (string, key [, algorithm] [, encoding] [, IVorSalt] [, iterations])
        // Encrypt/Decrypt/EncryptBinary/DecryptBinary: (string, key [, algorithm] [, encoding] [, IVorSalt] [, iterations])
        if (fname == "ENCRYPT" || fname == "DECRYPT" || fname == "ENCRYPTBINARY" || fname == "DECRYPTBINARY") {
            if (node->args.size() < 2 || node->args.size() > 6) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 2 to 6 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 6; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // ---- Tier-1 built-in functions (see CFFUNCTION_IMPLEMENTATION_ACTION_PLAN.md) ----

        // No-argument functions: getContextRoot(), getLocalHostIP(),
        // isDebugMode(), getSystemFreeMemory(), getSystemTotalMemory(),
        // getFunctionList(), getCSPNonce(), getClientVariablesList(),
        // transactionCommit().
        if (fname == "GETCONTEXTROOT" || fname == "GETLOCALHOSTIP" || fname == "ISDEBUGMODE" ||
            fname == "GETSYSTEMFREEMEMORY" || fname == "GETSYSTEMTOTALMEMORY" ||
            fname == "GETFUNCTIONLIST" || fname == "GETCSPNONCE" || fname == "GETCLIENTVARIABLESLIST" ||
            fname == "TRANSACTIONCOMMIT") {
            if (node->args.size() > 0 && !(node->args.size() == 1 && node->args[0]->type == ExprAST::LiteralNull)) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 0 arguments").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {});
        }

        // Single-argument functions: getEncoding(scope), getFreeSpace(path),
        // getTotalSpace(path), isIPv6(value), isLocalHost(value),
        // getMetricData(mode), isDDX(value), isWDDX(value),
        // deleteClientVariable(name), preserveSingleQuotes(variable),
        // createODBCDate(date), createODBCTime(date), isThreadInterrupted(name).
        if (fname == "GETENCODING" || fname == "GETFREESPACE" || fname == "GETTOTALSPACE" ||
            fname == "ISIPV6" || fname == "ISLOCALHOST" || fname == "GETMETRICDATA" ||
            fname == "ISDDX" || fname == "ISWDDX" || fname == "DELETECLIENTVARIABLE" ||
            fname == "PRESERVESINGLEQUOTES" || fname == "CREATEODBCDATE" ||
            fname == "CREATEODBCTIME" || fname == "ISTHREADINTERRUPTED") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {arg1});
        }

        // Two-argument function: objectEquals(object1, object2).
        if (fname == "OBJECTEQUALS") {
            if (node->args.size() != 2) {
                throw webstrada::exception("ObjectEquals requires exactly 2 arguments");
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_objectequals");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_objectequals", module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2});
        }

        // getToken(string, index [, delimiters]): 2 or 3 arguments.
        if (fname == "GETTOKEN") {
            if (node->args.size() < 2 || node->args.size() > 3) {
                throw webstrada::exception("GetToken requires 2 or 3 arguments");
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *arg3 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (node->args.size() == 3) {
                arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *fHelper = module->getFunction("cf_gettoken");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_gettoken", module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2, arg3});
        }

        // transactionRollback([savepoint]): 0 or 1 arguments.
        if (fname == "TRANSACTIONROLLBACK") {
            if (node->args.size() > 1) {
                throw webstrada::exception("TransactionRollback requires 0 or 1 arguments");
            }
            llvm::Value *arg1 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (node->args.size() == 1) {
                arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *fHelper = module->getFunction("cf_transactionrollback");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_transactionrollback", module
                );
            }
            return emitCall(builder, fHelper, {arg1});
        }

        // location(url [, addtoken] [, statuscode]): ColdFusion 2025 removed
        // the function form; the runtime throws CF's method-not-found error
        // with the actual argument count.
        if (fname == "LOCATION") {
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 3; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_location");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getInt32Ty()}, false),
                    llvm::Function::InternalLinkage, "cf_location", module
                );
            }
            return emitCall(builder, fHelper, {callArgs[0], callArgs[1], callArgs[2], builder.getInt32(static_cast<int>(node->args.size()))});
        }

        // setVariable(name, value): passes the eight scopes so the runtime can
        // assign dotted names (variables.x, form.x, ...) like <cfset>.
        if (fname == "SETVARIABLE") {
            if (node->args.size() != 2) {
                throw webstrada::exception("SetVariable requires exactly 2 arguments");
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_setvariable");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_setvariable", module
                );
            }
            llvm::Value *args[] = {arg1, arg2, cgi, server, cookie, application, session, url, form, variables};
            return emitCall(builder, fHelper, args);
        }

        // ---- Tier-2 encoder family (CFFUNCTION_IMPLEMENTATION_ACTION_PLAN.md) ----

        // encodeForHTML / encodeForHTMLAttribute / encodeForJavaScript /
        // encodeForCSS / encodeForXML / encodeForXMLAttribute / encodeForDN /
        // encodeForLDAP / encodeForXPath / decodeForHTML: 1 or 2 arguments
        // (the optional second is `canonicalize`; DecodeForHTML takes exactly 1).
        if (fname == "ENCODEFORHTML" || fname == "ENCODEFORHTMLATTRIBUTE" || fname == "ENCODEFORJAVASCRIPT" ||
            fname == "ENCODEFORCSS" || fname == "ENCODEFORXML" || fname == "ENCODEFORXMLATTRIBUTE" ||
            fname == "ENCODEFORDN" || fname == "ENCODEFORLDAP" || fname == "ENCODEFORXPATH") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 or 2 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // decodeForHTML(string): exactly 1 argument.
        if (fname == "DECODEFORHTML") {
            if (node->args.size() != 1) {
                throw webstrada::exception("DecodeForHTML requires exactly 1 argument");
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_decodeforhtml");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_decodeforhtml", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // canonicalize(input, restrictMultiple, restrictMixed [, throwOnError]):
        // 3 or 4 arguments.
        if (fname == "CANONICALIZE") {
            if (node->args.size() < 3 || node->args.size() > 4) {
                throw webstrada::exception("Canonicalize requires 3 or 4 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 4; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_canonicalize");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_canonicalize", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // iif(condition, expr1, expr2): exactly 3 arguments. The runtime needs
        // the scopes to evaluate the chosen expression dynamically.
        if (fname == "IIF") {
            if (node->args.size() != 3) {
                throw webstrada::exception("IIf requires exactly 3 arguments");
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg3 = CompileExprAST(module, builder, function, node->args[2], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_iif");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_iif", module
                );
            }
            llvm::Value *args[] = {arg1, arg2, arg3, currentOut(function), cgi, server, cookie, application, session, url, form, variables};
            return emitCall(builder, fHelper, args);
        }

        // isvalid(type, value [, min, max, pattern]): 2 to 5 arguments.
        if (fname == "ISVALID") {
            if (node->args.size() < 2 || node->args.size() > 5) {
                throw webstrada::exception("IsValid requires 2 to 5 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 5; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_isvalid");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_isvalid", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // getCpuUsage([interval]): 0 or 1 argument.
        if (fname == "GETCPUUSAGE") {            if (node->args.size() > 1) {
                throw webstrada::exception("GetCPUUsage requires 0 or 1 arguments");
            }
            llvm::Value *argVal = llvm::ConstantPointerNull::get(builder.getPtrTy());
            if (node->args.size() == 1) {
                argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            }
            auto *fHelper = module->getFunction("cf_getcpuusage");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_getcpuusage", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // getMetaData(object): exactly 1 argument.
        if (fname == "GETMETADATA") {
            if (node->args.size() != 1) {
                throw webstrada::exception("GetMetaData requires exactly 1 argument");
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_getmetadata");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_getmetadata", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // isOnline(value): exactly 1 argument.
        if (fname == "ISONLINE") {
            if (node->args.size() != 1) {
                throw webstrada::exception("isOnline requires exactly 1 argument");
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_isonline");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_isonline", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // invoke(object, methodName [, arguments]): 2 or 3 arguments. Passes
        // the live scopes so UDF lookup / component instantiation work.
        if (fname == "INVOKE") {
            if (node->args.size() < 2 || node->args.size() > 3) {
                throw webstrada::exception("invoke requires 2 or 3 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 3; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            callArgs.push_back(currentOut(function));
            callArgs.push_back(cgi);
            callArgs.push_back(server);
            callArgs.push_back(cookie);
            callArgs.push_back(application);
            callArgs.push_back(session);
            callArgs.push_back(url);
            callArgs.push_back(form);
            callArgs.push_back(variables);
            auto *fHelper = module->getFunction("cf_invoke");
            if (!fHelper) {
                std::vector<llvm::Type*> params(12, builder.getPtrTy());
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), params, false),
                    llvm::Function::InternalLinkage, "cf_invoke", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // ajaxLink(url): exactly 1 argument.
        if (fname == "AJAXLINK") {
            if (node->args.size() != 1) {
                throw webstrada::exception("ajaxLink requires exactly 1 argument");
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_ajaxlink");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_ajaxlink", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // ajaxOnLoad(functionName): exactly 1 argument; emits the inline
        // <script> to the page output, so it needs the out buffer.
        if (fname == "AJAXONLOAD") {
            if (node->args.size() != 1) {
                throw webstrada::exception("ajaxOnLoad requires exactly 1 argument");
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_ajaxonload");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_ajaxonload", module
                );
            }
            return emitCall(builder, fHelper, {argVal, currentOut(function)});
        }

        // queryExecute(sql [, params [, options]]): 1 to 3 arguments. Passes
        // the live scopes so the result / datasource options can be applied.
        if (fname == "QUERYEXECUTE") {
            if (node->args.size() < 1 || node->args.size() > 3) {
                throw webstrada::exception("queryExecute requires 1 to 3 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 3; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            callArgs.push_back(cgi);
            callArgs.push_back(server);
            callArgs.push_back(cookie);
            callArgs.push_back(application);
            callArgs.push_back(session);
            callArgs.push_back(url);
            callArgs.push_back(form);
            callArgs.push_back(variables);
            auto *fHelper = module->getFunction("cf_queryexecute");
            if (!fHelper) {
                std::vector<llvm::Type*> params(11, builder.getPtrTy());
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), params, false),
                    llvm::Function::InternalLinkage, "cf_queryexecute", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // isPDFFile(value) / isPDFObject(value): exactly 1 argument.
        if (fname == "ISPDFFILE" || fname == "ISPDFOBJECT") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string((fname + " requires exactly 1 argument").c_str()));
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // isPDFArchive(value [, standard]): 1 or 2 arguments.
        if (fname == "ISPDFARCHIVE") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception("IsPDFArchive requires 1 or 2 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_ispdfarchive");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_ispdfarchive", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // csvRead(data [, columns [, delimiter [, charset]]]): 1 to 4 args.
        if (fname == "CSVREAD") {
            if (node->args.size() < 1 || node->args.size() > 4) {
                throw webstrada::exception("CSVRead requires 1 to 4 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 4; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_csvread");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_csvread", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // csvWrite(data [, delimiter]): 1 or 2 args.
        if (fname == "CSVWRITE") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception("CSVWrite requires 1 or 2 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_csvwrite");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_csvwrite", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // csvProcess(data, callback [, delimiter]): 2 or 3 args. The callback
        // needs the scopes.
        if (fname == "CSVPROCESS") {
            if (node->args.size() < 2 || node->args.size() > 3) {
                throw webstrada::exception("CSVProcess requires 2 or 3 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 3; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_csvprocess");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_csvprocess", module
                );
            }
            llvm::Value *args[] = {callArgs[0], callArgs[1], callArgs[2], currentOut(function), cgi, server, cookie, application, session, url, form, variables};
            return emitCall(builder, fHelper, args);
        }

        // csrfGenerateToken([key [, forceNew]]): 0 to 2 args.
        if (fname == "CSRFGENERATETOKEN") {
            if (node->args.size() > 2) {
                throw webstrada::exception("CSRFGenerateToken requires 0 to 2 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_csrfgeneratetoken");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_csrfgeneratetoken", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // csrfVerifyToken(token [, key]): 1 or 2 args.
        if (fname == "CSRFVERIFYTOKEN") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception("CSRFVerifyToken requires 1 or 2 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_csrfverifytoken");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_csrfverifytoken", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // setProfileString(path, section, entry, value [, encoding]): 4 or 5 args.
        if (fname == "SETPROFILESTRING") {
            if (node->args.size() < 4 || node->args.size() > 5) {
                throw webstrada::exception("SetProfileString requires 4 or 5 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 5; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_setprofilestring");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_setprofilestring", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // getPropertyString(filePath, key [, encoding]): 2 or 3 args.
        if (fname == "GETPROPERTYSTRING") {
            if (node->args.size() < 2 || node->args.size() > 3) {
                throw webstrada::exception("GetPropertyString requires 2 or 3 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 3; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_getpropertystring");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_getpropertystring", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // getPropertyFile(filePath [, encoding]): 1 or 2 args.
        if (fname == "GETPROPERTYFILE") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception("GetPropertyFile requires 1 or 2 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_getpropertyfile");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_getpropertyfile", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // setPropertyString(filePath, keyOrMap [, value] [, encoding]): 2 to 4 args.
        if (fname == "SETPROPERTYSTRING") {
            if (node->args.size() < 2 || node->args.size() > 4) {
                throw webstrada::exception("SetPropertyString requires 2 to 4 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 4; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_setpropertystring");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_setpropertystring", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // getLocaleDisplayName([locale [, inLocale]]): 0 to 2 arguments.
        if (fname == "GETLOCALEDISPLAYNAME") {
            if (node->args.size() > 2) {
                throw webstrada::exception("GetLocaleDisplayName requires 0 to 2 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_getlocaledisplayname");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_getlocaledisplayname", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // getException(object): exactly 1 argument.
        if (fname == "GETEXCEPTION") {
            if (node->args.size() != 1) {
                throw webstrada::exception("GetException requires exactly 1 argument");
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_getexception");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_getexception", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // duplicate(object): exactly 1 argument.
        if (fname == "DUPLICATE") {
            if (node->args.size() != 1) {
                throw webstrada::exception("Duplicate requires exactly 1 argument");
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_duplicate");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_duplicate", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // objectSave(object [, file]): 1 or 2 args.
        if (fname == "OBJECTSAVE") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception("ObjectSave requires 1 or 2 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_objectsave");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_objectsave", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // objectLoad(binaryOrFile): exactly 1 argument.
        if (fname == "OBJECTLOAD") {
            if (node->args.size() != 1) {
                throw webstrada::exception("ObjectLoad requires exactly 1 argument");
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_objectload");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_objectload", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // numberFormat(number [, mask]): 1 or 2 arguments.
        if (fname == "NUMBERFORMAT") {            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception("NumberFormat requires 1 or 2 arguments");
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_numberformat");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_numberformat", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }


        // GenerateSecretKey: generateSecretKey(algorithm [, keysize])
        if (fname == "GENERATESECRETKEY") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 or 2 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_generatesecretkey");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_generatesecretkey", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // Generate3DesKey: generate3DesKey(seed)
        if (fname == "GENERATE3DESKEY") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_generate3deskey");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_generate3deskey", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // GeneratePBKDFKey: generatePBKDFKey(algorithm, passphrase, salt, iterations, keySize)
        if (fname == "GENERATEPBKDFKEY") {
            if (node->args.size() != 5) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 5 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 5; i++) {
                callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
            }
            auto *fHelper = module->getFunction("cf_generatepbkdfkey");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_generatepbkdfkey", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // FormatBaseN: formatBaseN(number, radix)
        if (fname == "FORMATBASEN") {
            if (node->args.size() != 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 2 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_formatbasen");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_formatbasen", module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2});
        }

        // HTMLCodeFormat / HTMLEditFormat: (string [, version])
        if (fname == "HTMLCODEFORMAT" || fname == "HTMLEDITFORMAT") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 or 2 arguments").c_str()));
            }
            auto *arg1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            llvm::Value *arg2 = nullptr;
            if (node->args.size() == 2) {
                arg2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            } else {
                arg2 = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
            std::string libName = "cf_" + node->op_val;
            for (auto &c : libName) c = tolower(c);
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, {arg1, arg2});
        }

        // ReplaceList: replaceList(string, list1, list2 [, delim1 [, delim2 [, includeEmptyFields]]])
        if (fname == "REPLACELIST") {
            if (node->args.size() < 3 || node->args.size() > 6) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 3 to 6 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 6; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_replacelist");
            if (!fHelper) {
                std::vector<llvm::Type*> params(6, builder.getPtrTy());
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), params, false),
                    llvm::Function::InternalLinkage, "cf_replacelist", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // ToBase64: toBase64(input [, encoding])
        if (fname == "TOBASE64") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 or 2 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_tobase64");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_tobase64", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // ToBinary: toBinary(input)
        if (fname == "TOBINARY") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_tobinary");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_tobinary", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // BinaryEncode: binaryEncode(binaryData, encoding)
        if (fname == "BINARYENCODE") {
            if (node->args.size() != 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 2 arguments").c_str()));
            }
            auto *argVal1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *argVal2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_binaryencode");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_binaryencode", module
                );
            }
            return emitCall(builder, fHelper, {argVal1, argVal2});
        }

        // URLDecode/URLEncodedFormat: (string [, charset])
        if (fname == "URLDECODE" || fname == "URLENCODEDFORMAT") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 or 2 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            std::string libName = (fname == "URLDECODE") ? "cf_urldecode" : "cf_urlencodedformat";
            auto *fHelper = module->getFunction(libName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, libName, module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // EncodeForURL: encodeForURL(string [, canonicalize])
        if (fname == "ENCODEFORURL") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 or 2 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_encodeforurl");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_encodeforurl", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // DecodeFromURL: decodeFromURL(string)
        if (fname == "DECODEFROMURL") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_decodefromurl");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_decodefromurl", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // URLSessionFormat: urlSessionFormat(requesturl) — not supported (needs session state)
        if (fname == "URLSESSIONFORMAT") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_urlsessionformat");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_urlsessionformat", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // CharsetDecode: charsetDecode(string, encoding) / CharsetEncode: charsetEncode(binaryData, encoding)
        if (fname == "CHARSETDECODE" || fname == "CHARSETENCODE") {
            if (node->args.size() != 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 2 arguments").c_str()));
            }
            auto *argVal1 = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *argVal2 = CompileExprAST(module, builder, function, node->args[1], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            std::string helperName = (fname == "CHARSETDECODE") ? "cf_charsetdecode" : "cf_charsetencode";
            auto *fHelper = module->getFunction(helperName);
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, helperName, module
                );
            }
            return emitCall(builder, fHelper, {argVal1, argVal2});
        }

        // ToScript: toScript(cfvar, javascriptvar [, outputformat] [, asformat])
        if (fname == "TOSCRIPT") {
            if (node->args.size() < 2 || node->args.size() > 4) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 2 to 4 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 4; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_toscript");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_toscript", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // ToString: toString(value [, encoding])
        if (fname == "TOSTRING") {
            if (node->args.size() < 1 || node->args.size() > 2) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires 1 or 2 arguments").c_str()));
            }
            std::vector<llvm::Value*> callArgs;
            for (size_t i = 0; i < 2; i++) {
                if (i < node->args.size()) {
                    callArgs.push_back(CompileExprAST(module, builder, function, node->args[i], cgi, server, cookie, application, session, url, form, variables, cfm_text));
                } else {
                    callArgs.push_back(llvm::ConstantPointerNull::get(builder.getPtrTy()));
                }
            }
            auto *fHelper = module->getFunction("cf_tostring");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_tostring", module
                );
            }
            return emitCall(builder, fHelper, callArgs);
        }

        // Val: val(string)
        if (fname == "VAL") {
            if (node->args.size() != 1) {
                throw webstrada::exception(webstrada::string(("Function " + node->op_val + " requires exactly 1 argument").c_str()));
            }
            auto *argVal = CompileExprAST(module, builder, function, node->args[0], cgi, server, cookie, application, session, url, form, variables, cfm_text);
            auto *fHelper = module->getFunction("cf_val");
            if (!fHelper) {
                fHelper = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false),
                    llvm::Function::InternalLinkage, "cf_val", module
                );
            }
            return emitCall(builder, fHelper, {argVal});
        }

        // Compiler-extension functions: a `__` prefix (reserved like C's
        // `__` identifiers) marks an engine-provided function that is compiled
        // as a direct JIT call to cf_<lowercased name>(args, argc) below —
        // never runtime-dispatched, never shadowable by a UDF/variable.
        const bool extensionCall = fname.size() > 2 && fname[0] == '_' && fname[1] == '_';

        std::vector<llvm::Value*> compiledArgs;
        // Named arguments (name=value) are collected into a marker struct passed
        // as args[0]; the remaining positional args follow in order. The runtime
        // (cf_udf_invoke / component invokes) reorders them against the declared
        // parameter names.
        {
            bool hasNamedArg = false;
            for (const auto &arg : node->args) {
                if (arg->type == ExprAST::NamedArg) { hasNamedArg = true; break; }
            }
            if (hasNamedArg) {
                auto *fSt = module->getFunction("cfvariant_create_struct");
                if (!fSt) fSt = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {}, false), llvm::Function::InternalLinkage, "cfvariant_create_struct", module);
                llvm::Value *namedStruct = emitCall(builder, fSt, {});
                auto *fSet = module->getFunction("cfvariant_index_assign");
                if (!fSet) fSet = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_index_assign", module);
                auto *fStr = module->getFunction("cfvariant_create_string");
                if (!fStr) fStr = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cfvariant_create_string", module);
                for (const auto &arg : node->args) {
                    if (arg->type == ExprAST::NamedArg) {
                        llvm::Value *valVal = CompileExprAST(module, builder, function, arg->right, cgi, server, cookie, application, session, url, form, variables, cfm_text);
                        llvm::Value *keyVal = emitCall(builder, fStr, {getPtrGlobalString(arg->string_val)});
                        emitCall(builder, fSet, {namedStruct, keyVal, valVal});
                    } else {
                        compiledArgs.push_back(CompileExprAST(module, builder, function, arg, cgi, server, cookie, application, session, url, form, variables, cfm_text));
                    }
                }
                auto *fMarker = module->getFunction("cf_named_args_marker");
                if (!fMarker) fMarker = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::InternalLinkage, "cf_named_args_marker", module);
                llvm::Value *namedMarker = emitCall(builder, fMarker, {namedStruct});
                compiledArgs.insert(compiledArgs.begin(), namedMarker);
            } else {
                for (const auto &arg : node->args) {
                    compiledArgs.push_back(CompileExprAST(module, builder, function, arg, cgi, server, cookie, application, session, url, form, variables, cfm_text));
                }
            }
        }

        llvm::Value *argArray = nullptr;
        if (!compiledArgs.empty()) {
            argArray = createEntryAlloca(builder, function, builder.getPtrTy(), builder.getInt32(compiledArgs.size()));
            for (size_t i = 0; i < compiledArgs.size(); i++) {
                auto *ptr = builder.CreateGEP(builder.getPtrTy(), argArray, builder.getInt32(i));
                builder.CreateStore(compiledArgs[i], ptr);
            }
        } else {
            argArray = llvm::ConstantPointerNull::get(builder.getPtrTy());
        }

        if (extensionCall) {
            // Only registered extension functions are callable: a `__name` that
            // is not in the engine's extension registry is a compile error
            // (the reserved namespace must not silently fall back to a UDF).
            if (!cfml::cf_is_extension_name(fname.c_str())) {
                throw webstrada::exception(webstrada::string("Unknown compiler extension function ") +
                                           fname.c_str() + ".");
            }
            std::string libName = "cf_" + fname;
            for (auto &c : libName) c = tolower(c);
            auto *fExt = module->getFunction(libName);
            if (!fExt) {
                fExt = llvm::Function::Create(
                    llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getInt32Ty()}, false),
                    llvm::Function::InternalLinkage, libName, module);
            }
            return emitCall(builder, fExt, {argArray, builder.getInt32(static_cast<int>(compiledArgs.size()))});
        }

        auto *f = module->getFunction("cfvariant_call_function");
        if (!f) {
            std::vector<llvm::Type*> p = {
                builder.getPtrTy(),
                builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy(),
                builder.getPtrTy(),
                builder.getPtrTy(),
                builder.getInt32Ty()
            };
            f = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), p, false), llvm::Function::InternalLinkage, "cfvariant_call_function", module);
        }

        llvm::Value *out = currentOut(function);

        llvm::Value *args[] = {
            out, cgi, server, cookie, application, session, url, form, variables,
            getPtrGlobalString(node->op_val), argArray, builder.getInt32(compiledArgs.size())
        };
        return emitCall(builder, f, args);
    }
    }
    return nullptr;
}

llvm::Value *CfmlExpressionToClang(
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *function,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie,
    llvm::Value *application, llvm::Value *session,
    llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const TextParserTokenItem &token,
    const char *cfm_text) {
    std::vector<TextParserTokenItem> children = token.children;
    const char *text = cfm_text;
    if (children.empty() && token.len > 0) {
        // Single-token expressions (e.g. a bare variable in <cfif x>) arrive
        // without children; re-tokenize the expression text so the condition
        // is not mistaken for an empty expression.
        string exprText(cfm_text + token.position, token.len);
        string wrapped = "<cfset __dummy = (" + exprText + ")>";
        parser p;
        p.parse(wrapped.constData(), wrapped.length(), TEXTPARSER_ENCODING_LATIN1);
        while (auto t = p.next_token()) {
            auto item = convertToken(t);
            if (item.token_id == TextParser_cfml_StartTag) {
                for (const auto &child : item.children) {
                    if (child.token_id == TextParser_cfml_Expression) {
                        children = child.children;
                        text = p.get_text();
                        break;
                    }
                }
            }
        }
    }
    auto ast = parseTokensToAST(children, text);
    return CompileExprAST(module, builder, function, ast, cgi, server, cookie, application, session, url, form, variables, text);
}

llvm::Value *CompileStringExpression(
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *function,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie,
    llvm::Value *application, llvm::Value *session,
    llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const string &exprStr,
    const char *cfm_text) {
    string wrapped = "<cfset __dummy = (" + exprStr + ")>";
    parser p;
    p.parse(wrapped.constData(), wrapped.length(), TEXTPARSER_ENCODING_LATIN1);
    std::vector<TextParserTokenItem> tokens;
    while (auto t = p.next_token()) {
        tokens.push_back(convertToken(t));
    }
    if (!tokens.empty() && tokens[0].token_id == TextParser_cfml_StartTag) {
        for (auto &child : tokens[0].children) {
            if (child.token_id == TextParser_cfml_Expression) {
                auto processedChildren = mergeObjectMembers(child.children);
                auto ast = parseTokensToAST(processedChildren, p.get_text());
                return CompileExprAST(module, builder, function, ast, cgi, server, cookie, application, session, url, form, variables, p.get_text());
            }
        }
    }
    auto *fNull = module->getFunction("cfvariant_create_null");
    if (!fNull) fNull = llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {}, false), llvm::Function::InternalLinkage, "cfvariant_create_null", module);
    return emitCall(builder, fNull, {});
}

} // namespace webstrada
