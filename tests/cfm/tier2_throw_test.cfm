<cfscript>
// throw() tests: message only, named args, positional args, custom types.
try {
    throw("hello world");
} catch (any e) {
    writeOutput("A:[" & e.type & "][" & e.message & "]");
}
try {
    throw(message="named msg", type="MyCustomType");
} catch (any e) {
    writeOutput("B:[" & e.type & "][" & e.message & "]");
}
try {
    throw("pos msg", "MyType2", "some detail", "errcode1", "extinfo");
} catch (any e) {
    writeOutput("C:[" & e.type & "][" & e.message & "][" & e.detail & "][" & e.errorcode & "][" & e.extendedinfo & "]");
}
// catch by exact custom type
try {
    throw(type="SpecialErr", message="boom");
} catch (SpecialErr e) {
    writeOutput("D:[" & e.message & "]");
}
// catch by application
try {
    throw(type="Application", message="app err");
} catch (Application e) {
    writeOutput("E:[" & e.message & "]");
}
// nested rethrow
try {
    try {
        throw("inner");
    } catch (any e) {
        throw;
    }
} catch (any e2) {
    writeOutput("F:[" & e2.message & "]");
}
// throw with expression message
try {
    throw("value " & (2 + 3));
} catch (any e) {
    writeOutput("G:[" & e.message & "]");
}
</cfscript>
