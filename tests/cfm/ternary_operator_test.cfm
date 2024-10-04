<cfscript>
writeOutput((1 ? "T" : "F"));
writeOutput("|" & (0 ? "A" : "B"));
writeOutput("|" & ("x" ? "C" : "D"));
writeOutput("|" & ("no" ? "E" : "F"));
writeOutput("|" & ("0" ? "G" : "H"));
writeOutput("|" & ("" ? "I" : "J"));
writeOutput("|" & ("false" ? "K" : "L"));
writeOutput("|" & ("yes" ? "M" : "N"));
writeOutput("|" & (true ? "O" : "P"));
writeOutput("|" & (0.0 ? "Q" : "R"));
writeOutput("|" & (0.5 ? "S" : "T"));
writeOutput("|" & (-1 ? "U" : "V"));
</cfscript>
<cfscript>
x = 2;
writeOutput("|" & (x EQ 2 ? "eq2" : "ne2"));
writeOutput("|" & (x GT 5 ? "big" : "small"));
writeOutput("|" & (x EQ 1 ? "one" : x EQ 2 ? "two" : "other"));
writeOutput("|" & (x EQ 1 ? "one" : (x EQ 2 ? "two" : "other")));
</cfscript>
<cfoutput>
|#(5 GT 3 ? "g" : "l")#|#(x EQ 2 ? "inout-eq2" : "inout-ne2")#
</cfoutput>
