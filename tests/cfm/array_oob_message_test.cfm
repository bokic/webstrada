<!--- array index out-of-bounds message (was BUGS.md #33) --->
<cfset killArray = []>
<cfset arr = [1,2,3]>
1:<cftry>
  <cfset x = killArray[1]>
  <cfcatch type="any"><cfoutput>[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|2:<cftry>
  <cfset x = arr[5]>
  <cfcatch type="any"><cfoutput>[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|3:<cftry>
  <cfset x = arr[0]>
  <cfcatch type="any"><cfoutput>[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|4:<cftry>
  <cfoutput>#arr[9]#</cfoutput>
  <cfcatch type="any"><cfoutput>[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
