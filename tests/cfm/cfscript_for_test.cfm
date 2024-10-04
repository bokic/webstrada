<cfscript>
for (i = 1; i <= 5; i++) {
    WriteOutput(i);
}
WriteOutput("|");
for (i = 5; i >= 1; i--) {
    WriteOutput(i);
}
WriteOutput("|");
for (i = 1; i <= 10; i += 2) {
    WriteOutput(i);
}
WriteOutput("|");
for (i = 10; i >= 1; i = i - 3) {
    WriteOutput(i);
}
WriteOutput("|");
for (i = 1; i < 3; i++) { }
WriteOutput("EMPTY");
WriteOutput("|");
sum = 0;
for (i = 1; i <= 4; i++) {
    sum = sum + i;
}
WriteOutput(sum);
</cfscript>
