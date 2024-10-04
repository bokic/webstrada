<cfscript>
c = createObject("component", "components/named_calc");
writeOutput(c.greet(who="CF"));
writeOutput("|" & c.add(b=2, a=3));
</cfscript>
