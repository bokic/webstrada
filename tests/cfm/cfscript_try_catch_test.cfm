<cfscript>
// 1. basic catch any, success path is skipped
try { a = 1/0; writeOutput("UNREACHABLE"); } catch (any e) { writeOutput("1:" & e.type & ":" & e.message); }
writeOutput("|");

// 2. no exception, catch skipped
try { writeOutput("2ok"); } catch (any e) { writeOutput("2err"); }
writeOutput("|");

// 3. undefined variable exception
try { b = undefinedVar1; } catch (any e) { writeOutput("3:" & e.type & ":" & e.message); }
writeOutput("|");

// 4. specific type matching, first match wins (application then expression)
try { c = 5/0; } catch (application e) { writeOutput("4app"); } catch (expression e) { writeOutput("4expr"); }
writeOutput("|");

// 5. finally runs on success
try { writeOutput("5t"); } finally { writeOutput("5f"); }
writeOutput("|");

// 6. finally runs after catch
try { d = 1/0; } catch (any e) { writeOutput("6c"); } finally { writeOutput("6f"); }
writeOutput("|");

// 7. nested try/catch inside a catch body
try { e = 1/0; } catch (any e) {
    writeOutput("7outer");
    try { f = undefinedVar2; } catch (any e2) { writeOutput("7inner:" & e2.type); }
}
writeOutput("|");

// 8. try/finally rethrows the unmatched exception to an outer catch
try {
    try { g = 1/0; } finally { writeOutput("8f"); }
} catch (any e) { writeOutput("8:" & e.type); }
writeOutput("|");

// 9. deeply nested finally chain, all run before the outer catch
try {
    try {
        try { h = 1/0; } finally { writeOutput("9a"); }
    } finally { writeOutput("9b"); }
} catch (any e) { writeOutput("9c"); }
writeOutput("|");

// 10. an exception raised inside finally replaces the original one
try {
    try { i = 1/0; } finally { j = undefinedVar3; }
} catch (any e) { writeOutput("10:" & e.type); }
writeOutput("|");

// 11. try/catch/finally inside a while loop
k = 0;
while (k < 3) {
    k++;
    try { if (k == 2) { l = 1/0; } writeOutput("11t" & k); }
    catch (any e) { writeOutput("11c" & k); }
    finally { writeOutput("11f" & k); }
}
writeOutput("|");

// 12. multiple catch clauses, most specific first
try { m = undefinedVar4; } catch (expression e) { writeOutput("12e"); } catch (any e) { writeOutput("12a"); }
writeOutput("|");

// 13. catch variable assigned inside the catch body is visible afterwards
try { n = 1/0; } catch (any e) { caughtName = e.type; }
writeOutput("13:" & caughtName);
writeOutput("|");

// 14. variable assigned inside finally is visible afterwards
try { o = 1; } finally { finVar = 42; }
writeOutput("14:" & finVar);
</cfscript>
