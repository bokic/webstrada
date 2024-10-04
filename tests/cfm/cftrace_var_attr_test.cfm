<!--- <cftrace var="..."> is processed even though `var` is a textparser Keyword
     (was BUGS.md "<cftrace var=...> attribute value is not evaluated"). CF
     treats var as a variable name: a literal name is a no-op with debugging
     disabled, and an undefined variable in a #...# expression throws (the
     setter runs even when debugging is off). --->
<cfset myvar = "hello">
<cftrace var="myvar" text="t">BODY</cftrace>
|LITERAL-NAME
<cftrace var="#myvar#">MID
|EXPR-NAME
<cftry><cftrace var="#zz_undef_var#"><cfcatch type="any"><cfoutput>CAUGHT:#cfcatch.message#</cfoutput></cfcatch></cftry>
|UNDEF
