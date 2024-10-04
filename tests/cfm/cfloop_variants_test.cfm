<cfloop list="a,b,c" index="it"><cfoutput>#it#</cfoutput></cfloop>
<cfoutput>|</cfoutput>
<cfloop list="a-b-c" delimiters="-" index="it"><cfoutput>#it#</cfoutput></cfloop>
<cfoutput>|</cfoutput>
<cfset arr2 = [7,8,9]>
<cfloop array="#arr2#" index="it"><cfoutput>#it#</cfoutput></cfloop>
<cfoutput>|</cfoutput>
<cfset st = {x:1,y:2}>
<cfloop collection="#st#" item="k"><cfoutput>#k#</cfoutput></cfloop>
<cfoutput>|</cfoutput>
<cfset i = 0>
<cfloop condition="i LT 3"><cfset i = i + 1><cfoutput>#i#</cfoutput></cfloop>
<cfoutput>|</cfoutput>
<cfloop index="i" from="1" to="3" step="1"><cfoutput>#i#</cfoutput></cfloop>
<cfoutput>|</cfoutput>
<cfloop list="#ListAppend('p','q')#" index="it"><cfoutput>#it#</cfoutput></cfloop>
