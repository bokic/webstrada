<cfscript>
// UDF exception handling: exceptions thrown inside UDFs must be catchable by
// the caller's try/catch (JIT-EH).

// 1. custom throw inside a UDF caught by the caller's script try/catch
function boom1() {
    throw(type="custom", message="m1", detail="d1");
}
try { boom1(); writeOutput("1UNREACHABLE"); }
catch (custom e) { writeOutput("1:" & e.type & ":" & e.message & ":" & e.detail); }
writeOutput("|");

// 2. type/message preserved through the UDF boundary, catch(any)
function boom2() {
    throw(type="fooBar", message="m2");
}
try { boom2(); }
catch (any e) { writeOutput("2:" & e.type & ":" & e.message); }
writeOutput("|");

// 3. runtime error (undefined variable) inside UDF caught by caller
function boom3() {
    x = undefinedVar1;
}
try { boom3(); }
catch (expression e) { writeOutput("3:" & e.type); }
writeOutput("|");

// 4. try/catch inside a UDF body catches a throw from a nested call
function inner() {
    throw(type="inner", message="deep");
}
function wrapper4() {
    try { inner(); } catch (inner e) { return "caught:" & e.message; }
}
writeOutput("4:" & wrapper4());
writeOutput("|");

// 5. finally inside a UDF runs before the exception reaches the caller
function withFinally() {
    try { throw(type="f", message="fm"); }
    finally { writeOutput("5fin;"); }
}
try { withFinally(); }
catch (f e) { writeOutput("5:" & e.message); }
writeOutput("|");

// 6. exception propagates through two nested UDF levels to the caller
function level3() {
    throw(type="l3", message="bottom");
}
function level2() {
    level3();
}
function level1() {
    level2();
}
try { level1(); }
catch (l3 e) { writeOutput("6:" & e.message); }
writeOutput("|");

// 7. a required argument missing inside a UDF is catchable by the caller
function reqarg(a) {
    return a;
}
try { reqarg(); }
catch (any e) { writeOutput("7:" & e.type); }
writeOutput("|");

// 8. multiple catch clauses in the caller match a UDF throw (custom type)
function boom8() {
    throw(type="custom8", message="e8");
}
try { boom8(); }
catch (custom8 e) { writeOutput("8cust"); }
catch (any e) { writeOutput("8any"); }
writeOutput("|");

// 9. rethrow inside a UDF propagates the original exception to the caller
function rethrower() {
    try { throw(type="rt", message="rmsg"); }
    catch (rt e) { rethrow; }
}
try { rethrower(); }
catch (rt e) { writeOutput("9:" & e.message); }
writeOutput("|");

// 10. a UDF called from a catch body can throw and be caught by an outer catch
function boom10() {
    throw(type="outer", message="o10");
}
try {
    try { x = 1 / 0; }
    catch (any e) { boom10(); }
}
catch (outer e) { writeOutput("10:" & e.message); }
writeOutput("|");

// 11. exception inside a UDF inside a loop is caught on each iteration
function boom11(i) {
    if (i eq 2) { throw(type="loop", message="at" & i); }
    writeOutput("11t" & i & ";");
}
for (k = 1; k <= 3; k++) {
    try { boom11(k); }
    catch (loop e) { writeOutput("11c" & k & ";"); }
}
writeOutput("|");

// 12. tag-form cffunction throwing, caught by script try/catch in the caller
</cfscript>
<cffunction name="tagboom" returntype="string">
    <cfthrow type="tagtype" message="tagmsg">
</cffunction>
<cfscript>
try { tagboom(); }
catch (tagtype e) { writeOutput("12:" & e.type & ":" & e.message); }
writeOutput("|");
</cfscript>

<cftry>
    <cfset tagboom()>
    <cfcatch type="tagtype">
        <cfoutput>13:#cfcatch.type#:#cfcatch.message#</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>|</cfoutput>

<cftry>
    <cfscript>
    // 14. tag-form cftry catching a script-form UDF throw
    function scriptboom() {
        throw(type="scripttype", message="smsg");
    }
    scriptboom();
    </cfscript>
    <cfcatch type="scripttype">
        <cfoutput>14:#cfcatch.message#</cfoutput>
    </cfcatch>
</cftry>
<cfoutput>|</cfoutput>

<cfscript>
// 15. recursion with exception at depth, caught at top
function deep(n) {
    if (n <= 0) { throw(type="deep", message="zero"); }
    return deep(n - 1);
}
try { deep(100); }
catch (deep e) { writeOutput("15:" & e.message); }
writeOutput("|");

// 16. closure throwing, caught by caller
makeThrowingClosure = function(msg) {
    return function() { throw(type="closuretype", message=msg); };
};
c = makeThrowingClosure("clmsg");
try { c(); }
catch (closuretype e) { writeOutput("16:" & e.message); }
</cfscript>
