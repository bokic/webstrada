<cfscript>
// Exception-scoped temp cleanup: values produced inside a try body that are
// assigned to variables must survive the scope cleanup (deep-copied strings/
// arrays, refcounted structs/queries); only the temp shells are freed.

// 1. struct assigned inside try survives outside (refcounted payload)
try {
    s = { a: "x", b: 42 };
    s.c = "y";
} catch (any e) { writeOutput("1err"); }
writeOutput("1:" & s.a & s.b & s.c);
writeOutput("|");

// 2. array assigned inside try survives (deep copy)
try {
    arr = [1, 2, 3];
    arr[2] = 99;
} catch (any e) { writeOutput("2err"); }
writeOutput("2:" & arrayLen(arr) & ":" & arr[2]);
writeOutput("|");

// 3. nested try/catch with cleanup at both levels
try {
    try {
        t1 = "outer";
        t2 = "inner";
        x = undefinedVar1;
    } catch (any e) {
        writeOutput("3inner;");
        t3 = "caught";
    }
    writeOutput(t1 & t2 & t3 & ";");
} catch (any e) { writeOutput("3outererr"); }
writeOutput("|");

// 4. try/catch inside a loop - temps must not accumulate or leak
for (i = 1; i <= 100; i++) {
    try {
        loopVal = "iter" & i;
        loopStruct = { n: i };
        throw(type="loop", message="x");
    } catch (loop e) {
        loopVal = loopVal & "-c";
    }
}
writeOutput("4:" & loopVal & ":" & loopStruct.n);
writeOutput("|");

// 5. query assigned inside try survives (refcounted)
try {
    q = queryNew("name", "varchar");
    queryAddRow(q);
    querySetCell(q, "name", "boris");
} catch (any e) { writeOutput("5err"); }
writeOutput("5:" & q.recordcount & ":" & q.name[1]);
writeOutput("|");

// 6. value created in try, returned through a UDF, used by caller
function makeFromTry() {
    try {
        inner = { k: "v", arr: [7, 8] };
        return inner;
    } catch (any e) { return "ERR"; }
}
r = makeFromTry();
writeOutput("6:" & r.k & ":" & r.arr[2]);
writeOutput("|");

// 7. finally creates a value that is used afterwards
try {
    f1 = "a";
} finally {
    f2 = "b";
    f3 = { z: 1 };
}
writeOutput("7:" & f1 & f2 & ":" & f3.z);
writeOutput("|");

// 8. exception path: catch reads only its own + captured exception
try {
    b1 = "before";
    x = undefinedVar2;
    b2 = "after";
} catch (any e) {
    writeOutput("8:" & b1 & ":" & e.type & ";");
}
writeOutput("|");

// 9. try body building a big struct that is then merged into a variable
try {
    big = {};
    for (j = 1; j <= 5; j++) { big["k#j#"] = j * 10; }
    merged = big;
} catch (any e) { writeOutput("9err"); }
writeOutput("9:" & merged.k3 & ":" & structCount(merged));
writeOutput("|");

// 10. try/catch where the catch throws a fresh exception (temp churn)
try {
    try { x = undefinedVar3; }
    catch (any e) { throw(type="rethrow2", message="fromcatch"); }
} catch (rethrow2 e) {
    writeOutput("10:" & e.message);
}
</cfscript>
