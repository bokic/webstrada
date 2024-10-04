<!--- <cfimage> attribute errors that ARE catchable on CF 2025; the ones that
abort the CF page (bad action, negative thickness, missing source, invalid
difficulty) are covered by unit tests (ImageCfimageTag* in tests/tests.cpp). --->
<cfset sep = "~">
<cfset im = ImageNew("", 4, 4, "rgb")>
<cftry><cfimage action="write" source="#im#" destination=""><cfoutput>#sep#NOERR1</cfoutput><cfcatch type="any"><cfoutput>#sep#E1:#cfcatch.type#|#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfimage action="read" source="#im#" name=""><cfoutput>#sep#NOERR2</cfoutput><cfcatch type="any"><cfoutput>#sep#E2:#cfcatch.type#|#cfcatch.message#</cfoutput></cfcatch></cftry>
