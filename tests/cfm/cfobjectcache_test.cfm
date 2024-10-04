<!--- Byte-verified against CF 2025. cfobjectcache action="clear" flushes the
query cache; a dynamic bad action is a catchable Template error (a static bad
action is a compile-time error, covered by unit tests). --->
<cfobjectcache action="clear">
<cfquery datasource="webstrada">
DROP TABLE IF EXISTS ocq_t
</cfquery>
<cfquery datasource="webstrada">
CREATE TABLE ocq_t (id INTEGER PRIMARY KEY, name TEXT)
</cfquery>
<cfquery datasource="webstrada">
INSERT INTO ocq_t VALUES (1, 'one')
</cfquery>
<cfquery name="q1" datasource="webstrada" cachedwithin="#CreateTimeSpan(1,0,0,0)#">
SELECT name FROM ocq_t WHERE id = 1
</cfquery>
<cfoutput>Q1=#q1.name#|</cfoutput>
<cfquery datasource="webstrada">
UPDATE ocq_t SET name = 'two' WHERE id = 1
</cfquery>
<cfquery name="q2" datasource="webstrada" cachedwithin="#CreateTimeSpan(1,0,0,0)#">
SELECT name FROM ocq_t WHERE id = 1
</cfquery>
<cfoutput>Q2CACHED=#q2.name#|</cfoutput>
<cfobjectcache action="clear">
<cfquery name="q3" datasource="webstrada" cachedwithin="#CreateTimeSpan(1,0,0,0)#">
SELECT name FROM ocq_t WHERE id = 1
</cfquery>
<cfoutput>Q3AFTERCLEAR=#q3.name#|</cfoutput>
<cftry><cfobjectcache action="clear"><cfoutput>OC1=OK|</cfoutput><cfcatch><cfoutput>OC1E=#cfcatch.message#</cfoutput></cfcatch></cftry>
<cfset ocBad = "bogus">
<cftry><cfobjectcache action="#ocBad#"><cfoutput>OC2=OK|</cfoutput><cfcatch><cfoutput>OC2E=#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfset ocEmpty = "">
<cftry><cfobjectcache action="#ocEmpty#"><cfoutput>OC3=OK|</cfoutput><cfcatch><cfoutput>OC3E=#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cftry><cfobjectcache action="clear"><cfset ocBodyZ = 55>SKIPPED</cfobjectcache><cfoutput>OC4=#IsDefined("ocBodyZ")#|</cfoutput><cfcatch><cfoutput>OC4E=#cfcatch.type#|#cfcatch.message#</cfoutput></cfcatch></cftry>
<cfoutput>END</cfoutput>
