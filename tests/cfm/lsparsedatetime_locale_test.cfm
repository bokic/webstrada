<!--- LSParseDateTime / LSIsDate locale-aware parsing (was BUGS.md #11) --->
<!--- Dates are serialized with DateFormat/TimeFormat rather than raw {ts} because
     CF's {ts} output century-pads years < 100 (year 20 prints as "2020"). --->
<cffunction name="P" output="true">
  <cfargument name="s"><cfargument name="loc" default="">
  <cftry>
    <cfset d = LSParseDateTime(arguments.s, arguments.loc)>
    <cfoutput>OK#DateFormat(d,"yyyy-mm-dd")#|#TimeFormat(d,"HH:mm:ss")#;</cfoutput>
    <cfcatch><cfoutput>THROW;</cfoutput></cfcatch>
  </cftry>
</cffunction>
<cffunction name="I" output="true">
  <cfargument name="s"><cfargument name="loc" default="">
  <cftry>
    <cfoutput>#LSIsDate(arguments.s, arguments.loc)#;</cfoutput>
    <cfcatch><cfoutput>ERR;</cfoutput></cfcatch>
  </cftry>
</cffunction>
<cfset old = SetLocale("German (Standard)")>
<cfoutput>
#P("15.05.2020")##P("15.05.20")##P("15.05.99")##P("15.05.70")##P("15.05.2020 10:30:00")##P("15.05.2020 10:30")#
|#P("10:30")##P("10:30:00")##P("10:30:00.123")##P("15. Mai 2020")##P("Fr, 15 Mai 2020 10:30:00 GMT")#
|#P("2020-05-15")##P("2020-05-15T10:30:00")##P("2020-05-15T10:30:00+05:00")##P("2020-05-15T10:30:00Z")##P("2020.05.15")#
|#P("2020-05-15T00:30:00+05:00")##P("2020-05-15T10:30:00-05:00")##P("Fri, 15 May 2020 10:30:00 GMT","English (US)")##P("2020-05-15T10:30:00")#
|#I("123")##I("")##I("abc")##I("12:30")##I("Mai")##I("15.05")##I("32.05.2020")##I("15.13.2020")##I("05/15/2020")##I("15.05.2020")#
|#I(CreateDate(2020,5,15))##I(3)#
|#P("05/15/2020","English (US)")##P("Sep 15, 2020","English (US)")##P("September 15, 2020","English (US)")##P("15/05/2020","English (US)")##P("5/15/2020 10:30 AM","English (US)")##P("Tuesday, May 15, 2020","English (US)")##P("15.05.20","English (US)")#
|#P("15/05/2020","French (Standard)")##P("15 mai 2020","French (Standard)")##P("15/05/2020 14:30","French (Standard)")##P("15/05/2020 14:30:00","French (Standard)")#
|#P("2020/05/15","Japanese")##P("2020年5月15日","Japanese")##P("令和2年5月15日","Japanese")#
|#P("2020. 5. 15","Korean")##P("2020년 5월 15일","Korean")##P("오전 10:30","Korean")#
|#P("2020-5-15","Chinese (China)")##P("2020年5月15日","Chinese (China)")##P("上午10:30","Chinese (China)")#
|#P("15 Mai 2020","English (UK)")##P("15/05/2020 14:30","English (UK)")#
</cfoutput>
