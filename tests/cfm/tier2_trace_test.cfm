<cfscript>
// trace() tests: no-op with debugging disabled, abort support, named args.
trace();
writeOutput("A:END;");
trace(text="hello trace", inline=true);
writeOutput("B:END;");
trace(text="showing x", inline=true);
writeOutput("C:END;");
trace(text="before abort");
trace(abort=true);
writeOutput("D:SHOULD NOT REACH");
</cfscript>
