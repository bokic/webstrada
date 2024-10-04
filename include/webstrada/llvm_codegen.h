#pragma once

#include "string.h"
#include "component.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include <functional>
#include <memory>
#include <vector>


namespace webstrada {

class worker;
class parser;

typedef std::function<void (string *out, void *cgi, void *server, void *cookie, void *application, void *session, void *url, void *form, void *variables)> template_fn;

class llvm_codegen {
public:
    llvm_codegen();
    template_fn compile(const string &pathname, bool print_ast = false);
    template_fn compile_string(const char *buffer, size_t size, bool print_ast = false, const char *name = "stdin");

    // Compiles a ColdFusion component (.cfc) into a ComponentInfo definition.
    // The JIT module (owning the method/body entry points) stays alive for the
    // lifetime of this compiler. The caller owns the returned ComponentInfo.
    ComponentInfo *compileComponent(const string &pathname);

private:
    template_fn compile_parsed(parser &parse, const char *name, bool print_ast);

    llvm::LLVMContext m_context;
    std::vector<std::unique_ptr<llvm::ExecutionEngine>> m_engines;
};

}
