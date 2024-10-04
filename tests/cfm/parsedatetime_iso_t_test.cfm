<!--- ParseDateTime ISO-8601 'T' separator and timezone offsets (was BUGS.md:
     "ParseDateTime() drops the time part of ISO T datetime strings"). Dates are
     serialized with DateFormat/TimeFormat rather than raw {ts} because CF's {ts}
     output century-pads years < 100. --->
<cffunction name="P" output="true">
  <cfargument name="s">
  <cftry>
    <cfset d = ParseDateTime(arguments.s)>
    <cfoutput>OK#DateFormat(d,"yyyy-mm-dd")#|#TimeFormat(d,"HH:mm:ss")#;</cfoutput>
    <cfcatch><cfoutput>THROW;</cfoutput></cfcatch>
  </cftry>
</cffunction>
<cfoutput>
#P("2020-05-15T10:30:00")#
|#P("2020-05-15T10:30:00Z")#
|#P("2020-05-15T10:30:00+05:00")#
|#P("2020-05-15T10:30:00-05:00")#
|#P("2020-05-15T00:30:00+05:00")#
|#P("2020-05-15T10:30:00+05:30")#
|#P("2020-05-15T10:30:00-0500")#
|#P("2020-05-15 10:30:00Z")#
|#P("2020-05-15 10:30:00")#
|#P("10:30:00")#
|#P("2020-05-15")#
|#P("2020-12-05")#
|#IsDate("2020-05-15T10:30:00Z")#|#IsDate("2020-05-15T10:30:00")#|#IsDate("2020-05-15T10:30:00+05:00")#|#IsDate("2020-05-15T10:30:00-05:00")#|#IsDate("2020-05-15")#|#IsDate("2020-12-05")#
</cfoutput>
