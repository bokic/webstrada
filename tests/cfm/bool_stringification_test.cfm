<cfset blit = true><cfset blit2 = false><cfset bcomp = (5 GT 3)><cfset bnot = not true><cfset bisnum = IsNumeric("5")><cfoutput>TAG1:#blit#|#bcomp#|#bnot#|#bisnum#</cfoutput><cfscript>
// cfscript string interpolation: literal booleans true/false, computed YES/NO
writeOutput("SCR1:#blit#|#bcomp#|#bnot#|#bisnum#|#true#|#(5 GT 3)#|#not true#|#IsNumeric('5')#");
writeOutput("<br>");
// & concatenation: literal booleans keep true/false, computed become YES/NO
writeOutput("C1:" & (true & true));
writeOutput("<br>");
writeOutput("C2:" & ("a" & true));
writeOutput("<br>");
writeOutput("C3:" & ("a" & false));
writeOutput("<br>");
writeOutput("C4:" & ("a" & blit));
writeOutput("<br>");
writeOutput("C5:" & ("a" & bcomp));
writeOutput("<br>");
writeOutput("C6:" & ("a" & bnot));
writeOutput("<br>");
writeOutput("C7:" & ("a" & bisnum));
writeOutput("<br>");
writeOutput("C8:" & (bcomp & true));
writeOutput("<br>");
writeOutput("C9:" & (blit & blit2));
writeOutput("<br>");
// and/or return their operands, preserving the literal/computed flavor
writeOutput("A1:" & (true AND false));
writeOutput("<br>");
writeOutput("A2:" & (false AND true));
writeOutput("<br>");
writeOutput("A3:" & (false OR true));
writeOutput("<br>");
writeOutput("A4:" & (bcomp OR true));
writeOutput("<br>");
writeOutput("A5:" & (true AND bcomp));
writeOutput("<br>");
writeOutput("A6:" & (bcomp AND true));
writeOutput("<br>");
writeOutput("A7:" & (1 AND 2));
writeOutput("<br>");
writeOutput("A8:" & (0 OR 2));
writeOutput("<br>");
writeOutput("A9:" & (1 EQ 1 AND true));
writeOutput("<br>");
writeOutput("A10:" & (true AND 1 EQ 1));
writeOutput("<br>");
writeOutput("A11:" & (not true AND false));
writeOutput("<br>");
// ToString follows the same literal/computed distinction
writeOutput("T1:" & ToString(true));
writeOutput("<br>");
writeOutput("T2:" & ToString(blit));
writeOutput("<br>");
writeOutput("T3:" & ToString(bcomp));
writeOutput("<br>");
writeOutput("T4:" & ToString(bnot));
writeOutput("<br>");
writeOutput("T5:" & ToString(bisnum));
</cfscript><cfscript>
// writeOutput of a computed boolean argument renders YES/NO
writeOutput("W1:");
writeOutput(blit);
writeOutput("|");
writeOutput(bcomp);
writeOutput("|");
writeOutput(bnot);
writeOutput("|");
writeOutput(bisnum);
</cfscript>
