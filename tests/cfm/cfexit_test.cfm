<!--- <cfexit>: exittag/exittemplate abort the page (see the cfexit_abort_*
      test files); method="loop" outside custom tags is a catchable error; a
      dynamically evaluated method dispatches at runtime; inside functions it
      returns undefined; inside includes/CFC construction bodies it stops only
      that boundary (all verified on CF 2025). --->
T1 loop catchable:
<cfset x = "loop">
<cftry><cfexit method="#x#">X<cfcatch type="any"><cfoutput>#cfcatch.type#:#cfcatch.message#</cfoutput></cfcatch></cftry>
|T2 static loop catchable:
<cftry><cfexit method="loop">X<cfcatch type="any"><cfoutput>#cfcatch.type#</cfoutput></cfcatch></cftry>
|T3 dynamic invalid method catchable:
<cfset y = "foo">
<cftry><cfexit method="#y#">X<cfcatch type="any"><cfoutput>#cfcatch.type#</cfoutput></cfcatch></cftry>
|T4 tag form inside function returns undefined:
<cffunction name="f" output="yes"><cfoutput>IN</cfoutput><cfexit><cfoutput>UNREACH</cfoutput></cffunction>
<cfoutput>#f() EQ ""#|</cfoutput>
|T5 loop inside function still catchable:
<cffunction name="g" output="yes"><cftry><cfexit method="loop"><cfcatch type="any"><cfoutput>#cfcatch.message#</cfoutput></cfcatch></cftry></cffunction>
<cfoutput>#g()#|</cfoutput>
|T6 script exit inside function:
<cfscript>function h() { writeOutput("SIN"); exit; writeOutput("SUNREACH"); } h(); writeOutput("|");</cfscript>
|T7 script exit in nested function returns innermost only:
<cfscript>function outer() { function inner() { exit; writeOutput("INNER_AFTER"); } inner(); writeOutput("OUTER_AFTER"); } outer();</cfscript>
|T8 include exits only the include:
BEFORE<cfinclude template="include_lib/cfexit_inc.cfm">AFTER
|T9 include script exit:
BEFORE<cfinclude template="include_lib/cfexit_inc_script.cfm">AFTER
|T10 include loop catchable:
BEFORE<cfinclude template="include_lib/cfexit_inc_loop.cfm">AFTER
|T11 CFC construction stops construction only:
<cfscript>c = new components.cfexit_comp(); writeOutput("|#c.name#");</cfscript>
|END
