<!--- array pop/shift/first/last empty-array + reverse-not-a-method edges (was BUGS.md #4) --->
<cfscript>
empty = [];
try { r = arrayPop(empty); writeOutput("1:[ok#r#]"); } catch (any e) { writeOutput("1:[#e.type#]#e.message#"); }
writeOutput("|");
try { r = arrayShift(empty); writeOutput("2:[ok#r#]"); } catch (any e) { writeOutput("2:[#e.type#]#e.message#"); }
writeOutput("|");
try { r = arrayFirst(empty); writeOutput("3:[ok#r#]"); } catch (any e) { writeOutput("3:[#e.type#]#e.message#"); }
writeOutput("|");
try { r = arrayLast(empty); writeOutput("4:[ok#r#]"); } catch (any e) { writeOutput("4:[#e.type#]#e.message#"); }
writeOutput("|");
a = [1,2,3];
try { r = a.reverse(); writeOutput("5:[ok#r#]"); } catch (any e) { writeOutput("5:[#e.type#]#e.message#"); }
</cfscript>
