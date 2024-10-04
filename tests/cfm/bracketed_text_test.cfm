<cfif 5 GT 3>X[T]Y<cfelse>[F]</cfif>|<cfif 1 LT 5>Q1</cfif>|
<cfoutput>P[Q]R</cfoutput>|
A[hello]B|
A(hello)B|
A{hi}B|
<cfif 1 GT 0>X[<cfset z = 5>]Y</cfif>|
[c]<cfif true>ok</cfif>[d]|
<cfset a = 1>X[Y]Z|
<cfswitch expression="1"><cfcase value="1">A[B]C</cfcase><cfdefaultcase>[D]</cfdefaultcase></cfswitch>|
<cfoutput>[#(2 + 3)#]</cfoutput>|
<cfif false>[N]<cfelse>[E]</cfif>|
[123]|A[1,2]B|[[x]]|A[hello|A]B|A[foo]bar[baz]|
{}|()|A{p}B|A(x)B|
<cfset s = "x">[v]|
<cfif 1 GT 5>X[T]Y<cfelse>[F]</cfif>|
A[<cfif true>b</cfif>]C|
