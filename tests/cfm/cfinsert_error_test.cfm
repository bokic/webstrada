<cftry>
  <cfset form.name = "alice">
  <cfset form.unknowncol = "x">
  <cfinsert datasource="webstrada" tablename="ci_users">
  <cfcatch type="any">
    <cfoutput>A[#cfcatch.message#]</cfoutput>
  </cfcatch>
</cftry>

<cftry>
  <cfset form.name = "ALICE">
  <cfupdate datasource="webstrada" tablename="ci_users">
  <cfcatch type="any">
    <cfoutput>|B[#cfcatch.message#]</cfoutput>
  </cfcatch>
</cftry>
