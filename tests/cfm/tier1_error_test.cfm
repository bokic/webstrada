<!--- Tier-1: error messages caught via cftry/cfcatch (verified vs CF 2025).
     TransactionCommit/Rollback are void functions, so they are called as
     cfscript statements (using them in #...# is a CF compile error). --->
<cfoutput>
1:<cftry>#GetToken("a,b,c,d",0)#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
|2:<cftry>#GetToken("a,b,c,d",-3)#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
|3:<cftry>#GetFreeSpace("/")#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
|4:<cftry>#GetTotalSpace("x")#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
|9:<cftry>#GetTemplatePath()#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
|10:<cftry>#ParameterExists("x")#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
|11:<cftry>#isThreadInterrupted("t1")#<cfcatch type="any">[#cfcatch.message#]</cfcatch></cftry>
</cfoutput>
<cftry><cfscript>TransactionCommit();</cfscript><cfcatch type="any"><cfoutput>|5:[#cfcatch.message#]</cfoutput></cfcatch></cftry>
<cftry><cfscript>TransactionRollback();</cfscript><cfcatch type="any"><cfoutput>|6:[#cfcatch.message#]</cfoutput></cfcatch></cftry>
