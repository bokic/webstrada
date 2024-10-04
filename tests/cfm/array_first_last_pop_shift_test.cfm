<!--- array pop/shift/first/last functions + member methods (was BUGS.md #4) --->
<cfscript>
a = [1, 2, 3];
writeOutput("1:[first=" & arrayFirst(a) & "|last=" & arrayLast(a) & "|still=" & a.toList() & "]");
b = [1, 2, 3];
r = arrayPop(b);
writeOutput("|2:[pop=" & r & "|now=" & b.toList() & "]");
c = [1, 2, 3];
r2 = arrayShift(c);
writeOutput("|3:[shift=" & r2 & "|now=" & c.toList() & "]");
d = [1, 2, 3];
writeOutput("|4:[mfirst=" & d.first() & "|mlast=" & d.last() & "|still=" & d.toList() & "]");
e = [1, 2, 3];
r3 = e.pop();
writeOutput("|5:[mpop=" & r3 & "|now=" & e.toList() & "]");
f = [1, 2, 3];
r4 = f.shift();
writeOutput("|6:[mshift=" & r4 & "|now=" & f.toList() & "]");
</cfscript>
