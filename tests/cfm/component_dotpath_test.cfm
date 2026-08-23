<cfoutput>START;</cfoutput>
<cfset caller = CreateObject("component", "components/subpkg/Caller")>
<cfoutput>#caller.callSub()#|</cfoutput>
<cfset caller2 = CreateObject("component", "components.subpkg.Caller")>
<cfoutput>#caller2.callSub()#|</cfoutput>
<cfset sub = CreateObject("component", "components.mylib.subcomp")>
<cfoutput>#sub.sub()#|</cfoutput>
<cfoutput>END</cfoutput>
