<!--- chain-base undefined-variable / element messages (was BUGS.md #32) --->
1:
<cftry>
  <cfif victim[1] eq 1>x</cfif>
  <cfcatch type="any"><cfoutput>1:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|2:
<cftry>
  <cfif undefinedStruct.key eq 1>x</cfif>
  <cfcatch type="any"><cfoutput>2:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|3:
<cftry>
  <cfif undefinedStruct["key"] eq 1>x</cfif>
  <cfcatch type="any"><cfoutput>3:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|4:
<cftry>
  <cfset r = undefinedStruct.key>
  <cfcatch type="any"><cfoutput>4:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|5:
<cftry>
  <cfset r2 = victim[1]>
  <cfcatch type="any"><cfoutput>5:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|6:
<cftry>
  <cfoutput>#undefinedStruct.key#</cfoutput>
  <cfcatch type="any"><cfoutput>6:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|7:
<cftry>
  <cfoutput>#victim[1]#</cfoutput>
  <cfcatch type="any"><cfoutput>7:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|8:
<cftry>
  <cfset r3 = undefinedStruct.a.b>
  <cfcatch type="any"><cfoutput>8:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|9:
<cftry>
  <cfoutput>#undefinedStruct.a.b#</cfoutput>
  <cfcatch type="any"><cfoutput>9:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|10:
<cftry>
  <cfscript>if (victim[1] eq 1) {}</cfscript>
  <cfcatch type="any"><cfoutput>10:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
|11:
<cftry>
  <cfloop condition="undefinedStruct.key eq 1"><cfbreak></cfloop>
  <cfcatch type="any"><cfoutput>11:[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
