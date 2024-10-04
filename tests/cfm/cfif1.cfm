<cfset score = 85 />
<cfset age = 20 />
<cfset name = "John" />
<cfoutput><cfif score GTE 90>A<cfelseif score GTE 80 AND age LT 25>B1<cfelseif score GTE 80>B2<cfelse>C</cfif>|<cfif NOT (name EQ "Bob") OR age EQ 21>NotBob<cfelse>IsBob</cfif>|<cfif 5 GT 3 AND 10 LTE 10 AND "apple" NEQ "orange">Match</cfif></cfoutput>
