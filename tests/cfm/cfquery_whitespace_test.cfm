<cfquery name="qa" datasource="webstrada" result="ra">
SELECT 1 AS one</cfquery>
<cfoutput>[#ra.SQL#]</cfoutput>
<cfquery name="qb" datasource="webstrada" result="rb">SELECT 2 AS two
</cfquery>
<cfoutput>[#rb.SQL#]</cfoutput>
<cfquery name="qd" datasource="webstrada" result="rd">SELECT   3 AS three</cfquery>
<cfoutput>[#rd.SQL#]</cfoutput>
<cfset x = 4>
<cfquery name="qe" datasource="webstrada" result="re">SELECT  #x#  AS four</cfquery>
<cfoutput>[#re.SQL#]</cfoutput>
<cfquery name="qf" datasource="webstrada" result="rf">SELECT 5
<cfset y = 6> AS five</cfquery>
<cfoutput>[#rf.SQL#]</cfoutput>
