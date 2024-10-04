P<cfoutput>X</cfoutput>
P<cfif true>Y</cfif>
<cfset wsA = 1>Q
P<cfif true>Y</cfif>
<cfset wsB = 1>Q
P<cfoutput>X</cfoutput>
<cfset wsC = 1>P<cfif true>Y</cfif>
<cfset wsD = 1>Q
P<cfoutput>X</cfoutput>
<cfif false><cfset wsE = 1></cfif>
P<cfif true>Y</cfif>
<cfset wsF = 1>Q
P<cfoutput>X</cfoutput>
<cfif true><cfoutput>W</cfoutput></cfif>
P<cfif true>Y</cfif>
<cfset wsG = 1>Q
P<cfset x1 = "V">
<cfoutput>#x1#</cfoutput>
<cfset wsH = 1>Q
P<cfset x2 = "V#chr(10)#">
<cfoutput>#x2#</cfoutput>
<cfset wsI = 1>Q
P<cfset x3 = "V">
<cfoutput>#x3#
</cfoutput>
P<cfif true>Y</cfif>
<cfset wsJ = 1>Q
P<cfoutput>
Z
</cfoutput>
P<cfif true>Y</cfif>
<cfset wsK = 1>Q
P<cfscript>writeOutput("S");</cfscript>
P<cfif true>Y</cfif>
<cfset wsL = 1>Q
P<cfoutput>WO</cfoutput>
<cfset wsM = 1>
<cfset wsN = 2>
<cfset wsO = 3>P<cfif true>Y</cfif>
<cfset wsP = 1>Q
P<cfoutput>WO</cfoutput>
<cfif true>
<cfoutput>WO2</cfoutput>
</cfif>
<cfset wsQ = 1>Q
P<cfif true>
Y
</cfif>
<cfset wsR = 1>Q
