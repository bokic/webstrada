#pragma once

// Internal declarations shared between the LLVM code-gen translation units
// (src/codegen/llvm_codegen.cpp + src/codegen/codegen_*.cpp) and the compiler
// driver (src/codegen/llvm_compiler.cpp). Not part of the public webstrada API.

#include <webstrada/string.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <textparser.hpp>
#include <cfml_definition.json.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace webstrada {

inline bool isOperatorToken(int token_id) {
    return token_id >= TextParser_cfml_AssignOperator && token_id <= TextParser_cfml_Operator;
}

struct TextParserTokenItem
{
    std::vector<TextParserTokenItem> children;
    int token_id = 0;
    size_t position = 0;
    size_t len = 0;
    std::string error;
};

struct TextParserTokenItemKeyValue
{
    std::map<std::string, TextParserTokenItem> params;
    int token_id = 0;
    size_t position = 0;
    size_t len = 0;
    std::string error;
};

struct ExprAST {
    enum Type {
        LiteralNull,
        LiteralInt,
        LiteralLong,
        LiteralFloat,
        LiteralBool,
        LiteralString,
        Variable,
        UnaryOp,
        BinaryOp,
        FuncCall,
        VarIndex,
        Assign,
        ArrayLiteral,
        StructLiteral,
        Increment,
        Closure,
        MemberCall,
        NewExpr,
        TernaryExpr,
        NamedArg
    } type;

    int int_val = 0;
    long long long_val = 0;
    double float_val = 0.0;
    bool bool_val = false;
    std::string string_val;
    std::string op_val;
    // Raw literal text for float literals ("8.0", "8.10", "5.0E2"); empty
    // for computed values. CF preserves the literal text of float literals.
    std::string float_literal_text;

    TextParserTokenItem token;
    std::unique_ptr<ExprAST> left;
    std::unique_ptr<ExprAST> right;
    std::vector<std::unique_ptr<ExprAST>> args;
    // Array literal elements (ArrayLiteral) and struct literal values
    // (StructLiteral); structLiteralKeys holds the matching struct keys.
    std::vector<std::unique_ptr<ExprAST>> elements;
    std::vector<std::string> structLiteralKeys;
    std::vector<std::unique_ptr<ExprAST>> structLiteralValues;
    // True when the node was produced by a parenthesized sub-expression. CF
    // rejects indexing/member access directly on a parenthesized expression
    // ((arr)[1], (s).a), so a following VarIndex or '.' must throw.
    bool fromParen = false;
    // Increment (++) / decrement (--): true for pre-increment (++x), false
    // for post-increment (x++). op_val is "+" or "-".
    bool isPre = false;
    // True when a Variable node is the base of a member access (s.x) or array
    // index (arr[1]). Such a variable must never fall back to a built-in
    // method handle when undefined (CF: pi.foo -> "Variable PI is undefined.",
    // not a handle), so the handle fallback is disabled for these.
    bool isChainBase = false;
    // True when this '.' BinaryOp is itself the base of a further member/index
    // access (a.b.c -> the inner a.b is chained). The chain-base ELEMENT error
    // message only applies to a terminal single-dot access (CF: a.b on an
    // undefined a -> "Element B is undefined in A.", but a.b.c -> "Variable A
    // is undefined.").
    bool isChainedMember = false;
    // Closure nodes: parameter names/types/defaults and the raw CodeBlock
    // children of the body (compiled later into a JIT function).
    std::vector<std::string> closureParams;
    std::vector<std::string> closureParamTypes;
    std::vector<std::vector<TextParserTokenItem>> closureParamDefaults;
    std::vector<TextParserTokenItem> closureBody;
};

struct TryCatchClause {
    webstrada::string type;
    webstrada::string varName;
};

// Compilation state shared across the codegen files (defined in llvm_codegen.cpp).
extern thread_local struct EhContext *g_ehContext;
extern thread_local llvm::Value *g_currentCatchExn;
extern thread_local llvm::Value *g_currentOut;
extern thread_local llvm::BasicBlock *g_currentFuncCleanupBB;
extern thread_local bool g_compileInFunctionBody; // true while compiling a UDF/closure body (var is allowed)
// True while compiling the inside of a <cfoutput> body. Plain-text writes
// OUTSIDE <cfoutput> are emitted through the cfoutputonly-gated writers so
// <cfsetting enablecfoutputonly> suppresses them like CF; <cfoutput> content
// must never be gated.
extern thread_local bool g_insideCfoutput;
// The textparser handle of the template currently being compiled (set in
// compile_parsed / compileComponent). Its line map feeds
// textparser_get_line_number_at_position, which lineOfOffset uses instead of
// scanning the raw text.
extern thread_local textparser_t g_textparser;
extern int g_closureCounter;
// Per-template (module) compile-time-bound variable slots: (function, name) →
// LLVM alloca holding a VarFastSlot. Filled lazily as page-level variable
// reads are compiled; cleared at the start of each template compile. Keyed by
// function because a template's main function and a component construction
// body compile into different LLVM functions that cannot share an alloca.
// Used by the fast-path cfvariant_get_var_fast / cfvariant_bare_identifier_fast.
extern thread_local std::map<std::pair<llvm::Function*, std::string>, llvm::Value*> g_varFastSlots;
struct FunctionReturnCtx {
    llvm::Value *retSlot = nullptr;
    llvm::BasicBlock *exitBB = nullptr;
    std::string returnType;
    std::string funcName;
};
extern FunctionReturnCtx *g_returnCtx;
struct EhContext { llvm::BasicBlock *landingPadBB; };

struct UdfDef {
    std::string name;
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTypes;
    std::vector<std::vector<TextParserTokenItem>> paramDefaults;
    std::vector<bool> paramRequired;
    std::string returnType;
    std::vector<TextParserTokenItem> bodyTokens;
    bool isClosure = false;
    bool isTagForm = false;
    bool output = true;
    std::string access = "public";
    std::vector<std::string> paramDefaultsRaw;
    std::vector<bool> paramHasDefaultRaw; // tag-form: the default attribute was present (even if empty)
    size_t bodyStart = 0;
    size_t bodyEnd = 0;
};

enum class WsContent { None, PlainText, Tag };
enum class WsRight { Tag, Comment, PlainText, DocumentEnd };

struct WsFlag {
    bool arm = false;
    bool sticky = false;
};

struct WhitespaceState {
    bool enabled;
    WsFlag &flag;
    bool active = false;
    WsContent left = WsContent::None;
    std::string ws;
    WsContent last = WsContent::None;

    WhitespaceState(bool en, WsFlag &f);

    bool hasNewline() const;

    static void emitRuntimeSpace(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out);

    void resolve(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out, WsRight right);
    void feed(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out,
              const char *text, size_t len, WsRight right);
    void finish(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out, WsRight right);
    void markTag(bool arm, bool sticky);
};

struct LoopInfo {
    llvm::BasicBlock *incBB;
    llvm::BasicBlock *endBB;
    string indexName;
    llvm::AllocaInst *indexVar;
    bool isSwitch = false;
};

llvm::Value *currentOut(llvm::Function *function);

llvm::Value *emitCall(llvm::IRBuilder<> &builder, llvm::Function *f,
                      llvm::ArrayRef<llvm::Value*> args);

llvm::Function *getOrCreateHelper(llvm::Module *module, llvm::IRBuilder<> &builder,
                                  const char *name, llvm::Type *retTy,
                                  std::vector<llvm::Type*> params);

// Allocates a stack slot in the function's ENTRY block, no matter where the
// builder currently is. LLVM allocas placed in a loop/try body execute once
// per iteration and never pop, so the stack grows without bound in hot loops
// (was BUGS.md "Temp-variant accumulation ... layout-sensitive heap
// corruption": a throw-per-iteration try/catch loop exhausted the 16 MB
// stack). All our allocas have constant sizes, so hoisting is always safe.
inline llvm::AllocaInst *createEntryAlloca(llvm::IRBuilder<> &builder, llvm::Function *fn,
                                           llvm::Type *ty, llvm::Value *arraySize = nullptr) {
    llvm::IRBuilderBase::InsertPoint ip = builder.saveIP();
    builder.SetInsertPoint(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    auto *a = builder.CreateAlloca(ty, arraySize);
    builder.restoreIP(ip);
    return a;
}

TextParserTokenItem convertToken(const textparser_token_item *src);

void collectFunctionDecls(const std::vector<TextParserTokenItem> &tokens,
                          const char *cfm_text, std::vector<UdfDef> &out);
void collectTagFunctionDecls(const std::vector<TextParserTokenItem> &tokens,
                             const char *cfm_text, std::vector<UdfDef> &out);

// Parses a tag-form <cffunction> declaration (attributes + <cfargument>s +
// body span). Used by the component compiler too.
size_t parseTagFunctionDecl(const std::vector<TextParserTokenItem> &tokens,
                            size_t start, const char *cfm_text, UdfDef &def);

std::string buildUdfMetaBlob(const UdfDef &def, const char *cfm_text);

llvm::Function *compileUdfFunction(
    llvm::Module *module,
    llvm::LLVMContext &context,
    llvm::IRBuilder<> &builder,
    const std::string &llvmName,
    const UdfDef &def,
    const char *cfm_text,
    size_t cfm_text_size,
    bool isComponentMethod = false,
    const char *stackFnName = nullptr);

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

// ---- promoted from llvm_codegen.cpp (split) ----

llvm::Value *CfmlExpressionToClang(
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *function,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie,
    llvm::Value *application, llvm::Value *session,
    llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const TextParserTokenItem &token,
    const char *cfm_text
);

llvm::Value *CompileExprAST(
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *function,
    const std::unique_ptr<ExprAST> &node,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie,
    llvm::Value *application, llvm::Value *session,
    llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const char *cfm_text
);

llvm::Value *CompileStringExpression(
    llvm::Module *module,
    llvm::IRBuilder<> &builder,
    llvm::Function *function,
    llvm::Value *cgi, llvm::Value *server, llvm::Value *cookie,
    llvm::Value *application, llvm::Value *session,
    llvm::Value *url, llvm::Value *form, llvm::Value *variables,
    const string &exprStr,
    const char *cfm_text
);

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

// ---- CFML call-stack (stacktrace) codegen helpers ----
// Push a frame for the current module's name (the template pathname) onto the
// runtime call stack; `functionName` is the uppercased function/method name
// ("" for a plain template or component construction body). Update the top
// frame's line before a statement; pop on exit.
// cf_stack_capture_on_exception snapshots the live stack into an in-flight
// exception the moment its first landing pad sees it.
void emitStackPush(llvm::Module *module, llvm::IRBuilder<> &builder, const char *functionName = "");
void emitStackSetLine(llvm::Module *module, llvm::IRBuilder<> &builder, int line);
void emitStackPop(llvm::Module *module, llvm::IRBuilder<> &builder);
void emitStackCaptureOnException(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *exn);

// 1-based line number of the byte offset `pos` in `text`. Uses the active
// textparser's line map (textparser_get_line_number_at_position on
// g_textparser, which is encoding-independent); falls back to scanning `text`
// for '\n' when no compile is active. 1 when pos is out of range.
int lineOfOffset(const char *text, size_t pos);

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
    std::vector<LoopInfo> &loopStack);


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
    std::vector<LoopInfo> &loopStack);


// <cfsavecontent> (codegen_tags.cpp): pushes a runtime capture buffer, compiles
// the body into it (its whitespace managed like <cfsilent>) and assigns the
// captured text to the `variable` name. The body is wrapped in a catch-all so
// an exception still stores the partial content (CF's SaveContentTag.doCatch)
// before propagating.
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
    std::vector<LoopInfo> &loopStack);


// <cfprocessingdirective> (codegen_tags.cpp): validates the (compile-time,
// literal) suppressWhitespace/pageEncoding attributes and compiles the body
// with whitespace management enabled/disabled accordingly (CF's
// pushWSManagementSetting). suppressWhitespace="no" keeps the body's whitespace
// verbatim; absent or yes uses the default management.
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
    std::vector<LoopInfo> &loopStack);


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
    std::vector<LoopInfo> &loopStack);


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
    std::vector<LoopInfo> &loopStack);


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
    std::vector<LoopInfo> &loopStack);

// <cfcache> (codegen_tags.cpp): emits the runtime begin call, and for the body
// form compiles the body only on a cache miss (jumping over it on a hit) then
// the end-tag store; for the self-closing form returns from the page when the
// whole page was served from cache (CF's SKIP_PAGE).
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
    std::vector<LoopInfo> &loopStack);

// <cfstoredproc> (codegen_tags.cpp): pushes a runtime call context, compiles the
// body (whose <cfprocparam>/<cfprocresult> append to it) into a discard buffer,
// then executes the procedure and assigns the result sets / out values /
// status / execution-time variables.
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
    std::vector<LoopInfo> &loopStack);

// <cfdirectory> (codegen_tags.cpp): validates the static ACTION at compile
// time, builds the attribute struct and calls cf_directory_tag.
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
    std::vector<LoopInfo> &loopStack);

// <cfzipparam> (codegen_tags.cpp): appends a parameter to the active <cfzip>
// context via cf_zip_param.
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
    std::vector<LoopInfo> &loopStack);

// <cfzip> (codegen_tags.cpp): validates the static ACTION at compile time,
// pushes the zip context, compiles the body (whose <cfzipparam> children append
// to it) into a discard buffer, then executes the action via cf_zip_end.
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
    std::vector<LoopInfo> &loopStack);

// <cffile> (codegen_tags.cpp): validates the static ACTION at compile time,
// builds the attribute struct, captures the write/append tag body (skipped for
// static non-write/append actions, like CF's SKIP_BODY) and calls cf_file_tag.
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
    std::vector<LoopInfo> &loopStack);

// <cfexecute> (codegen_tags.cpp): validates the attribute set (name required,
// unknown attributes are compile-time errors like CF) and calls cf_execute_tag.
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
    std::vector<LoopInfo> &loopStack);

// <cffeed> (codegen_tags.cpp): validates the attribute set and calls
// cf_feed_tag with the attrs struct + scopes.
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
    std::vector<LoopInfo> &loopStack);

// <cfwddx> (codegen_tags.cpp): validates the attribute set (action required,
// input required for a static action, a static action value validated against
// CFML2WDDX/CFML2JS/WDDX2CFML/WDDX2JS) and calls cf_wddx_tag.
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
    std::vector<LoopInfo> &loopStack);


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
    const std::function<void()> &compileFinally);

string extractVarFromSharpExpr(const TextParserTokenItem &sharpExpr, const char *text);

llvm::Function *getOrCreatePersonality(llvm::Module *module, llvm::IRBuilder<> &builder,
                                              llvm::Function *fn);

llvm::Function *get_cfevalbool_func(llvm::Module *module, llvm::IRBuilder<> &builder);

llvm::Function *get_response_add_header_func(llvm::Module *module, llvm::IRBuilder<> &builder);

llvm::Function *get_response_redirect_func(llvm::Module *module, llvm::IRBuilder<> &builder);

bool isTextFormatCfdump(const string &tagText);

bool kwTextIs(const TextParserTokenItem &t, const char *cfm_text, const char *kw);

void llvm_CfAbort(llvm::Module *module, llvm::IRBuilder<> &builder);

void llvm_CfContent(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out,
                           llvm::Value *type, llvm::Value *reset, llvm::Value *file,
                           llvm::Value *variable, llvm::Value *deletefile);

void llvm_CfFlush(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out);

void llvm_CfGetVar(llvm::Module *module, llvm::IRBuilder<> &builder,
                           llvm::Value *scope, llvm::Value *key, llvm::Value *&result);

void llvm_CfOutputExpr(llvm::Module *module, llvm::IRBuilder<> &builder,
                               llvm::Value *out, llvm::Value *cgi, llvm::Value *server,
                               llvm::Value *cookie, llvm::Value *application, llvm::Value *session,
                               llvm::Value *url, llvm::Value *form, llvm::Value *variables,
                               llvm::Value *varName);

void llvm_WriteOutput(llvm::Module *module, llvm::IRBuilder<> &builder, llvm::Value *out, const char *str, size_t n);

std::string lowercase(const std::string &s);

std::vector<TextParserTokenItem> mergeObjectMembers(const std::vector<TextParserTokenItem> &children);

size_t parseFunctionDecl(const std::vector<TextParserTokenItem> &tokens, size_t start,
                                const char *cfm_text, UdfDef &def);

void parseParamList(const TextParserTokenItem &parenToken, const char *cfm_text,
                           std::vector<std::string> &names,
                           std::vector<std::string> &types,
                           std::vector<std::vector<TextParserTokenItem>> &defaults);

std::vector<std::pair<std::string, std::vector<TextParserTokenItem>>>
parseTagAttrs(const std::vector<TextParserTokenItem> *attrParts, const char *cfm_text);

void parseTagAttrs(const TextParserTokenItem &startTag, const char *cfm_text,
                          std::map<std::string, std::string> &out);

std::unique_ptr<ExprAST> parseTokensToAST(const std::vector<TextParserTokenItem> &tokens, const char *cfm_text, bool sharpContext = false);

std::map<string, string> parse_attributes(const string &tagText);

std::string tagNameOf(const TextParserTokenItem &t, const char *cfm_text);

std::string tokenText(const TextParserTokenItem &t, const char *cfm_text);

std::string uppercase(const std::string &s);

void validateOutputExpressionSharp(const TextParserTokenItem &outputExpression, const char *cfm_text);

} // namespace webstrada
