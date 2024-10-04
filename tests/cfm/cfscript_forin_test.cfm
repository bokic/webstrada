<cfscript>
arr = [10, 20, 30];
for (item in arr) {
    WriteOutput(item);
}
WriteOutput("|");
st = {a: 1, b: 2};
for (key in st) {
    WriteOutput(key);
}
WriteOutput("|");
lst = "a,b,c";
for (item in lst) {
    WriteOutput("[#item#]");
}
WriteOutput("|");
s = "hello";
for (ch in s) {
    WriteOutput("[#ch#]");
}
WriteOutput("|");
for (i = 1; i <= 5; i++) {
    if (i == 2) continue;
    if (i == 4) break;
    WriteOutput(i);
}
WriteOutput("|");
st2 = StructNew();
StructInsert(st2, "foo", 1);
for (k in st2) WriteOutput("[#k#]");
</cfscript>
