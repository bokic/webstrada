<cfset variables.dateformat = "shadowed-string" />
<cfset variables.timeformat = "shadowed-string" />
<cfset variables.len = "shadowed-string" />
<cfset variables.ucase = "shadowed-string" />
<cfoutput>#dateformat("2026-08-25 05:01:05","mm/dd/yyyy")#|#timeformat("2026-08-25 05:01:05","HH:MM")#|#len("abc")#|#ucase("abc")#</cfoutput>
