<!--- cfexecute: process execution, output capture, arguments tokenization. --->
<cftry>
<cfexecute name="/bin/echo" arguments="hello world" timeout="5" variable="res">
<cfoutput>VAR=[#res#]</cfoutput>
<cfcatch type="any"><cfoutput>ERR:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<cftry>
<cfexecute name="/bin/echo" arguments="-n noNewline" timeout="5" variable="res2">
<cfoutput>|NONL=[#res2#]</cfoutput>
<cfcatch type="any"><cfoutput>ERR2:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- arguments with quoted spaces --->
<cftry>
<cfexecute name="/bin/echo" arguments='-n "quoted arg" plain' timeout="5" variable="res3">
<cfoutput>|QUOTED=[#res3#]</cfoutput>
<cfcatch type="any"><cfoutput>ERR3:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- array arguments --->
<cftry>
<cfset argArr = ["-n", "array arg"]>
<cfexecute name="/bin/echo" arguments="#argArr#" timeout="5" variable="res4">
<cfoutput>|ARR=[#res4#]</cfoutput>
<cfcatch type="any"><cfoutput>ERR4:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- stderr to variable --->
<cftry>
<cfexecute name="/bin/sh" arguments='-c "echo ERRMSG 1>&2"' timeout="5" variable="res5" errorvariable="evar">
<cfoutput>|OUT=[#res5#]EV=[#evar#]</cfoutput>
<cfcatch type="any"><cfoutput>ERR5:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- outputfile --->
<cftry>
<cfexecute name="/bin/echo" arguments="to file" outputfile="#GetTempDirectory()#cfexecute_out.txt" timeout="5">
<cffile action="read" file="#GetTempDirectory()#cfexecute_out.txt" variable="fileContent"></cffile>
<cfoutput>|FILE=[#fileContent#]</cfoutput>
<cfcatch type="any"><cfoutput>ERR6:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- no output (process produces nothing) --->
<cftry>
<cfexecute name="/bin/true" timeout="5" variable="res7" errorvariable="ev7">
<cfoutput>|EMPTY:[#res7#]:[#ev7#]</cfoutput>
<cfcatch type="any"><cfoutput>ERR7:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- timeout --->
<cftry>
<cfexecute name="/bin/sleep" arguments="3" timeout="1" variable="res8">
<cfoutput>NOERR8</cfoutput>
<cfcatch type="any"><cfoutput>|TIMEOUT:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

<!--- executable not found --->
<cftry>
<cfexecute name="/nonexistent/cmd" arguments="x" timeout="5" variable="res9">
<cfoutput>NOERR9</cfoutput>
<cfcatch type="any"><cfoutput>|NOTFOUND:#cfcatch.type#:#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch>
</cftry>

<!--- page output (no variable) --->
<cftry>
<cfexecute name="/bin/echo" arguments="PAGE OUT" timeout="5">
<cfcatch type="any"><cfoutput>ERR10:#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch>
</cftry>
<cfoutput>|DONE</cfoutput>
