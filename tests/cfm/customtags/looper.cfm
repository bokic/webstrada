<cfif thisTag.executionMode eq "start">
  <cfparam name="attributes.times" default="3">
  <cfset caller.loopCount = 0>
<cfelse>
  <cfset caller.loopCount = caller.loopCount + 1>
  <cfif caller.loopCount lt attributes.times>
    <cfexit method="loop">
  </cfif>
</cfif>
