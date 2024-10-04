<cfscript>
// 1. throw string message -> Application
try { throw "hello"; } catch (application e) { writeOutput("1:" & e.type & ":" & e.message); }
writeOutput("|");

// 2. throw number message
try { throw 42; } catch (any e) { writeOutput("2:" & e.message); }
writeOutput("|");

// 3. throw parenthesized expression
try { throw (5+3); } catch (any e) { writeOutput("3:" & e.message); }
writeOutput("|");

// 4. bare throw -> Application, empty message
try { throw; } catch (any e) { writeOutput("4:" & e.type & ":" & e.message); }
writeOutput("|");

// 5. throw() -> Application, empty message
try { throw(); } catch (any e) { writeOutput("5:" & e.type & ":" & e.message); }
writeOutput("|");

// 6. throw message from variable
m = "abc"; try { throw m; } catch (any e) { writeOutput("6:" & e.message); }
writeOutput("|");

// 7. throw function-call result
try { throw ucase("xyz"); } catch (any e) { writeOutput("7:" & e.message); }
writeOutput("|");

// 8. named attributes, custom type preserved
try { throw(type="myType", message="named"); } catch (myType e) { writeOutput("8:" & e.type & ":" & e.message); }
writeOutput("|");

// 9. full attribute set
try { throw(type="My.Custom.Type", message="x", detail="d", errorcode="7", extendedinfo="ei"); }
catch (my.custom.type e) { writeOutput("9:" & e.type & ":" & e.message & ":" & e.detail & ":" & e.errorcode & ":" & e.extendedinfo); }
writeOutput("|");

// 10. custom type does not match a different catch
try { throw(type="t1", message="nomatch"); } catch (t2 e) { writeOutput("10wrong"); } catch (any e) { writeOutput("10:" & e.type & ":" & e.message); }
writeOutput("|");

// 11. rethrow inside catch re-raises to the next enclosing catch
try {
    try { throw "original"; }
    catch (application e) { writeOutput("11inner:" & e.message & ";"); rethrow; }
    writeOutput("11UNREACHABLE");
} catch (any e) { writeOutput("11outer:" & e.message); }
writeOutput("|");

// 12. rethrow targets the innermost enclosing catch
try {
    try { throw "orig"; }
    catch (application inner) {
        try { throw "second"; }
        catch (application innermost) { rethrow; }
        writeOutput("12UNREACHABLE");
    }
    writeOutput("12UNREACHABLE2");
} catch (any e) { writeOutput("12:" & e.message); }
writeOutput("|");

// 13. throw inside loop with catch
for (i = 1; i <= 3; i++) {
    try { if (i == 2) throw "boom"; writeOutput("13:" & i & ";"); }
    catch (any e) { writeOutput("13caught:" & e.message & ";"); }
}
writeOutput("|");

// 14. throw type/message do not fall through, next statement after try runs
try { throw "after"; } catch (any e) { writeOutput("14:" & e.message & ";"); }
writeOutput("done");
</cfscript>
