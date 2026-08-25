<cfimport prefix="mytag" taglib=".">
<cfset callervar = "inner_val">
<cfif thisTag.executionMode eq "start">
[NP-START]
<cfoutput>[NP-HAS:#structKeyExists(caller, "callervar")#:#caller.callervar#]</cfoutput>
<cfoutput>[NP-VAR:#structKeyExists(variables, "callervar")#]</cfoutput>
<mytag:simple name="Inner" />
<cfelse>
[NP-END]
</cfif>
