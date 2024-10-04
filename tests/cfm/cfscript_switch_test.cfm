<cfscript>
x = 2;
switch (x) {
    case 1:
        WriteOutput("ONE");
        break;
    case 2:
        WriteOutput("TWO");
        break;
    default:
        WriteOutput("DEFAULT");
}
WriteOutput("|");
x = 5;
switch (x) {
    case 1:
        WriteOutput("ONE");
        break;
    case 2:
        WriteOutput("TWO");
        break;
    default:
        WriteOutput("DEFAULT");
}
WriteOutput("|");
x = 1;
switch (x) {
    case 1:
        WriteOutput("FALL1");
    case 2:
        WriteOutput("FALL2");
    default:
        WriteOutput("FALLD");
}
WriteOutput("|");
switch ("b") {
    case "a":
        WriteOutput("A");
        break;
    case "b":
    case "c":
        WriteOutput("BC");
        break;
    default:
        WriteOutput("D");
}
WriteOutput("|");
y = "text";
switch (y) {
    case "other":
        WriteOutput("O");
        break;
    case "TEXT":
        WriteOutput("T");
        break;
}
WriteOutput("|");
z = 3;
switch (z) {
    case 1:
    case 2:
        WriteOutput("LOW");
        break;
    case 3:
        WriteOutput("HIGH");
        break;
}
WriteOutput("|");
e = 9;
switch (e) {
    case 1:
        WriteOutput("NOMATCH");
        break;
}
WriteOutput("NODEFAULT");
</cfscript>
