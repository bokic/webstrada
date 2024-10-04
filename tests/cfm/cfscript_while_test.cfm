<cfscript>
i = 0;
while (i < 3) {
    WriteOutput(i);
    i++;
}
WriteOutput("|");
j = 0;
do {
    WriteOutput("DO");
} while (false);
WriteOutput("|");
k = 0;
do {
    k++;
    if (k == 2) continue;
    WriteOutput(k);
} while (k < 4);
WriteOutput("|");
m = 0;
while (m < 1) {
    WriteOutput("W");
    m++;
}
</cfscript>
