#pragma once

#include "string.h"

#include <cxxabi.h>
#include <string>
#include <vector>


namespace webstrada
{

// One entry of the CFML call stack (the "stacktrace"): the full pathname of
// the template currently being executed, the current line within it and the
// name of the function/method being executed (empty for a plain template or a
// component construction body). The JIT pushes a frame at every template /
// UDF / component-method / component-construction entry, updates the top line
// before each statement and pops on every exit (normal return or exception
// unwinding), so an exception always sees the exact frame chain that produced
// it. Order is outermost (request template) first, innermost (where the error
// occurred) last.
struct StackLevel {
    std::string path;
    int line = 0;
    std::string function;   // uppercased function/method name ("" for a page)
};

// Runtime CFML exception. The C++ exception is thrown by runtime helpers and
// unwound by the Itanium ABI through the JIT frames; the JIT's try/catch
// landing pads inspect the caught object through these fields to implement CFML
// `catch (type)` matching.
class exception: public std::exception {
public:
    exception() : m_type("Expression") {}
    exception(const string &message, const string &detail = "")
        : m_type("Expression")
        , m_message(message)
        , m_detail(detail)
        {}
    exception(const string &type, const string &message, const string &detail)
        : m_type(type)
        , m_message(message)
        , m_detail(detail)
        {}

    // The CFML call stack captured when this exception was thrown (the first
    // JIT landing pad the unwinding exception reaches snapshots the live call
    // stack into the in-flight exception object; later landing pads skip a
    // frame that was already captured). Empty when the exception was created
    // but never unwound through a landing pad (e.g. an internal C++ error that
    // was caught before reaching CFML). Used to build the TAGCONTEXT array and
    // the error page's "The error occurred in ..." line.
    std::vector<StackLevel> m_stackTrace;

    // ColdFusion exception type used for `catch (type)` matching. Built-in
    // types use ColdFusion's canonical capitalization ("Expression",
    // "Application", "Request", "Template"); user `cfthrow` types keep their
    // original casing.
    string m_type;

    // True when the exception was raised by a user `throw`/`<cfthrow>` with a
    // custom (non-reserved) type name, or by a bare throw. CF matches custom
    // exceptions by exact type name (and dotted-prefix), NOT by class
    // hierarchy; built-in runtime errors are matched by class distance.
    bool m_isCustom = false;

    string m_message;
    string m_detail;

    // A CFML error code set by `<cfthrow errorcode="...">` / the equivalent
    // script form. Empty when not provided.
    string m_errorCode;

    // A CFML extended info string set by `<cfthrow extendedinfo="...">`.
    string m_extendedInfo;

    // True when this is a top-level "template not found" error (the request
    // page itself could not be resolved). In CF this is a
    // TemplateNotFoundException extending java.io.FileNotFoundException, which
    // is NOT a subclass of MissingIncludeException: it never reaches the
    // <cferror> exception handlers (only the built-in 404 page).
    bool m_missingTemplate = false;

    // Whether a CFML `catch` block may intercept this exception. `<cfabort>`
    // is NOT catchable in ColdFusion (only onRequestEnd sees it), so
    // abort_exception overrides this to false.
    virtual bool catchable() const { return true; }

    virtual const char* what() const noexcept override { return m_message.constData(); }
};

// Thrown by cfabort/cf_abort to halt template processing. Never caught by a
// CFML catch block; it must keep unwinding to the request handler.
class abort_exception: public exception {
public:
    abort_exception() { m_type = "Request"; }
    bool catchable() const override { return false; }
};

// Thrown by <cfexit> / script `exit;` (method exittag/exittemplate, the
// default) outside a function body to abort the currently executing template
// page. Like abort_exception it is never caught by a CFML catch block, but
// unlike abort it is swallowed at page boundaries so only the current page
// stops: the <cfinclude> machinery exits the included page and returns to the
// caller, the CFC construction body stops there and the instantiation
// continues, and the Application.cfm prelude stops without affecting the page.
// At the top level the request handler halts the page (output preserved).
class exit_exception: public exception {
public:
    exit_exception() { m_type = "Template"; }
    bool catchable() const override { return false; }
};

// Thrown for template-level errors (e.g. an uncaught runtime error surfaced at
// the request boundary). Catchable like CF's "template" type.
class template_exception: public exception {
public:
    template_exception() { m_type = "Template"; }
};

}

namespace cfml
{

using webstrada::cfvariant;

// Extracts the CFML exception struct (type/message/detail/errorcode/
// extendedinfo) from the Itanium exception object pointed to by `exn` (the
// landing pad's element 0) and returns a new Struct variant holding
// TYPE/MESSAGE/DETAIL/ERRORCODE/EXTENDEDINFO/TAGCONTEXT keys. Marks the
// exception as caught (`__cxa_begin_catch`) and destroys it (`__cxa_end_catch`)
// so a caught CFML exception never leaks. The returned struct is registered as
// a temp variant.
//
// This is the single runtime entry point the JIT uses to turn a caught C++
// exception into a CFML-visible value; the raw exception object must not be
// touched afterwards.
cfvariant *cf_eh_capture(void *exn);

// Whether a caught exception (`exn`, landing pad element 0) matches a CFML
// catch type clause. "any" matches every catchable exception; otherwise the
// exception's type must equal `cfType` case-insensitively. Uncatchable
// exceptions (cfabort) never match. The exception object is left untouched.
//
// NOTE: this function is kept for compatibility with existing callers; the
// JIT try/catch dispatch uses cf_eh_best_match instead (CF matches by closest
// exception-class ancestor across ALL clauses, not first-match).
bool cf_eh_matches(const void *exn, const char *cfType);

// Implements ColdFusion's `findThrowableTarget`/`findCustomTarget`: returns the
// index of the catch clause (from `types[0..count)`) whose target class is the
// closest ancestor of the thrown exception's class — "any" always matches (as
// java.lang.Exception), ties are broken by clause order — or -1 when nothing
// matches. Custom exceptions (user throw/cfthrow with a non-reserved type) are
// matched by exact type name (and dotted-prefix) over `any`, exactly like CF's
// findCustomTarget. Uncatchable exceptions (cfabort) never match.
int cf_eh_best_match(const void *exn, const char **types, int count);

// Number of superclass transitions from the exception class of `type` up to
// `target`, or INT_MAX when `target` is not an ancestor (CF's diffClassTypes
// over the engine's class hierarchy: java.lang.Exception root <- RuntimeException
// <- NeoException <- {ExpressionException, ApplicationException <-
// TemplateException <- {MissingIncludeException, CustomException}, DatabaseException,
// LockException, AccessControlException, ObjectException, SearchEngineException}).
// `target` is the class a <cferror exception="..."> handler registered for.
int cf_eh_class_distance(const char *cls, const char *target);

// The exception class a built-in (non-custom) thrown type maps to; unknown
// engine-internal types map to NeoException (catchable by `expression`/`any`).
const char *cf_eh_thrown_class(const webstrada::string &type);

// Throws a fresh runtime exception built from the TYPE/MESSAGE/DETAIL/
// ERRORCODE/EXTENDEDINFO fields of the struct `ex`. Used by `rethrow`/
// `<cfrethrow>` and by a `finally` block that must re-raise an uncaught
// exception (captured earlier by cf_eh_capture).
    [[noreturn]] void cf_eh_throw(cfvariant *ex);

// Fresh user throw (`<cfthrow>` / script `throw`): applies CF 2025's
// reserved-type validation (a throw of a built-in type name is replaced by an
// ApplicationException with "Attribute validation error ...") and marks
// custom throws so catch matching uses exact-name semantics. `isFunction`
// selects the script ("function throw") vs tag ("tag CFTHROW") message.
    [[noreturn]] void cf_eh_throw_new(cfvariant *ex, int isFunction);

}
