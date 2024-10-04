<!--- script throw of a complex object (was BUGS.md #7) --->
<cfscript>
s = {a:1};
try { throw s; } catch (any e) { writeOutput("1:[#e.type#]#e.message#"); }
writeOutput("|");
a = [1,2,3];
try { throw a; } catch (any e) { writeOutput("2:[#e.type#]#e.message#"); }
writeOutput("|");
q = queryNew("x");
queryAddRow(q);
querySetCell(q, "x", 1);
try { throw q; } catch (any e) { writeOutput("3:[#e.type#]#e.message#"); }
</cfscript>
