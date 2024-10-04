<cfscript>
x = 2;
if (x == 1) {
    WriteOutput("ONE");
} else if (x == 2) {
    WriteOutput("TWO");
} else if (x == 3) {
    WriteOutput("THREE");
} else {
    WriteOutput("OTHER");
}
WriteOutput("|");
if (x == 1) WriteOutput("ONE2");
else if (x == 2) WriteOutput("TWO2");
else WriteOutput("OTHER2");
WriteOutput("|");
y = 7;
if (y GT 10) {
    WriteOutput("BIG");
} else if (y GT 5) {
    WriteOutput("MID");
} else {
    WriteOutput("SMALL");
}
</cfscript>
