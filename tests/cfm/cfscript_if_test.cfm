<cfscript>
// Single-line if/else statement bodies (no braces)
if (true) WriteOutput("A");
if (false) WriteOutput("B");
WriteOutput("C");
if (false) WriteOutput("D"); else WriteOutput("E");
if (true) WriteOutput("F"); else WriteOutput("G");
if (false) WriteOutput("H"); else if (true) WriteOutput("I"); else WriteOutput("J");
if (false) WriteOutput("K"); else if (false) WriteOutput("L"); else if (true) WriteOutput("M"); else WriteOutput("N");
if (false) WriteOutput("O"); else if (false) WriteOutput("P"); else WriteOutput("Q");
if (false) WriteOutput("R"); else if (true) WriteOutput("S");
// Mixed block and single-statement bodies
if (true) { WriteOutput("T"); } else WriteOutput("U");
if (false) WriteOutput("V"); else { WriteOutput("W"); }
// Nested single-line ifs with dangling-else binding
if (true) if (true) WriteOutput("X"); else WriteOutput("Y"); else WriteOutput("Z");
if (true) if (false) WriteOutput("AA"); else WriteOutput("AB"); else WriteOutput("AC");
if (false) if (true) WriteOutput("AD"); else WriteOutput("AE"); else WriteOutput("AF");
// Multi-statement block bodies
if (true) { WriteOutput("AG"); WriteOutput("AH"); } else { WriteOutput("AI"); }
// Empty bodies
if (true) { } else WriteOutput("AJ");
if (false) WriteOutput("AK");
WriteOutput("AL");
// Assignments in single-line branches
v = 15;
if (v GT 20) r = "gt"; else if (v GT 10) r = "mid"; else r = "lt";
WriteOutput(r);
if (v LT 20) s = "small"; else s = "big";
WriteOutput(s);
</cfscript>
