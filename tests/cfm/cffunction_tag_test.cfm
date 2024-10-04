<cffunction name="callMe" returntype="numeric" output="false">
    <cfargument name="x" type="numeric" required="true">
    <cfreturn x * 2>
</cffunction>
<cfoutput>HOIST_BEFORE:#callMe(5)#</cfoutput>
|
<cfoutput>HOIST_AFTER:#callMe(21)#</cfoutput>
|
<cffunction name="withDefaults" output="false">
    <cfargument name="a" type="numeric" default="10">
    <cfargument name="b" type="string" default="hello">
    <cfargument name="c" default="#1+1#">
    <cfreturn a & "_" & b & "_" & c>
</cffunction>
<cfoutput>DEFAULTS:#withDefaults()#|#withDefaults(7)#</cfoutput>
|
<cffunction name="echoOut" output="false">
    <cfargument name="s" type="string" required="true">
    <cfoutput>Echoing #s#</cfoutput>
    <cfreturn s>
</cffunction>
|
<cffunction name="echoOutTrue" output="true">
    <cfargument name="s" type="string" required="true">
    <cfoutput>EchoingTrue #s#</cfoutput>
    <cfreturn s>
</cffunction>
|
<cffunction name="noOutAttr">
    <cfargument name="a" default="1">
    <cfoutput>OUT_#a#</cfoutput>
    <cfreturn a>
</cffunction>
<cfoutput>OUTF:[#echoOut("AAA")#]</cfoutput>
<cfset r2 = echoOutTrue("BBB")>
<cfoutput>OUTT:#r2#</cfoutput>
<cfoutput>DEFOUT:[#noOutAttr()#]</cfoutput>
|
<cffunction name="localscope" output="false">
    <cfargument name="p" type="numeric" required="true">
    <cfset var q = p * 3>
    <cfreturn q & ":" & p>
</cffunction>
<cfoutput>LOCAL:#localscope(4)#</cfoutput>
|
<cffunction name="noreturn" output="false">
    <cfargument name="z" type="numeric" default="99">
</cffunction>
<cfoutput>NORETURN:[#noreturn()#]</cfoutput>
|
<cffunction name="controlFlow" output="false">
    <cfargument name="n" type="numeric" required="true">
    <cfif n GT 10>
        <cfreturn "big">
    <cfelseif n GT 5>
        <cfreturn "mid">
    <cfelse>
        <cfreturn "small">
    </cfif>
</cffunction>
<cfoutput>CTRL:#controlFlow(20)#,#controlFlow(7)#,#controlFlow(1)#</cfoutput>
|
<cffunction name="leaktest" output="false">
    <cfargument name="p">
    <cfset leakvar = p & "_X">
    <cfreturn leakvar>
</cffunction>
<cfoutput>LEAK:#leaktest("A")#,#leakvar#</cfoutput>
|
<cffunction name="argkeys" output="false">
    <cfargument name="abc">
    <cfreturn StructKeyList(arguments) & "|" & StructFind(arguments,"ABC")>
</cffunction>
<cfoutput>ARGKEYS:#argkeys(1,2,3)#</cfoutput>
|
<cffunction name="typed" returntype="numeric" output="false">
    <cfargument name="n" type="string" required="true">
    <cfreturn n>
</cffunction>
<cfoutput>TYPED:#typed("42")#</cfoutput>
|
<cffunction name="outer" output="false">
    <cfargument name="n">
    <cffunction name="inner">
        <cfargument name="x">
        <cfreturn x * 2>
    </cffunction>
    <cfreturn inner(n)>
</cffunction>
<cfoutput>NESTED:#outer(6)#</cfoutput>
|
<cffunction name="writesViaOutput" output="true">
    <cfset writeOutput("TWO ")>
    <cfreturn "RET">
</cffunction>
<cfoutput>WO:#writesViaOutput()#</cfoutput>
