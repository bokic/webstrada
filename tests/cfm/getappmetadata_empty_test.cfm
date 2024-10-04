<!--- GetApplicationMetadata: empty struct without <cfapplication> (was BUGS.md #3) --->
<cfset m = GetApplicationMetadata()>
<cfoutput>1:[#structCount(m)#|#structKeyList(m)#]</cfoutput>
