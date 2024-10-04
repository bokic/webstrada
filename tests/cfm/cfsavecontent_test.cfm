<cfsavecontent variable="x">
Hello <cfoutput>#1+1#</cfoutput>
</cfsavecontent>
<cfoutput>[#x#]</cfoutput>
<cfset y = "">
<cfsavecontent variable="y">  spaces <cfset z = 5>  </cfsavecontent>
<cfoutput>[#y#]</cfoutput>
<cfsavecontent variable="a.b">dotted</cfsavecontent>
<cfoutput>[#a.b#]</cfoutput>
<cfset n = "dyn">
<cfsavecontent variable="#n#">dynamic</cfsavecontent>
<cfoutput>[#dyn#]</cfoutput>
<cfsavecontent variable="multi">
<cfloop from="1" to="3" index="i"><cfoutput>#i#</cfoutput></cfloop>
</cfsavecontent>
<cfoutput>[#multi#]</cfoutput>
