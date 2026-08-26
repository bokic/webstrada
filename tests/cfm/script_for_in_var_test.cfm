<cfscript>
function runTest() {
    items = [ { type = "page" }, { type = "post" } ];
    result = "";
    for (var panel in items) {
        result &= panel.type & ":";
    }
    base = { first = "A", second = "B" };
    seenFirst = false;
    seenSecond = false;
    for (var key in base) {
        if (local.key == "first" && base[local.key] == "A") seenFirst = true;
        if (local.key == "second" && base[local.key] == "B") seenSecond = true;
    }
    writeOutput(result & seenFirst & seenSecond);
}
runTest();
</cfscript>
