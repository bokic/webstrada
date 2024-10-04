<cfset s1 = REReplace("hello","l","-")>
<cfset s2 = REReplace("hello","(l)","[\1]","all")>
<cfset s3 = REFind("l","hello")>
<cfset s4 = REFind("l","hello",1,true)>
<cfset a1 = REMatch("l","hello")>
<cfset a2 = REMatchNoCase("[a-z]","HELLO")>
<cfset s5 = REReplaceNoCase("HELLO","[a-z]","X","all")>
<cfset s6 = ReEscape("a.b&c")>
<cfoutput>
[#s1#][#s2#][#s3#][#ArrayToList(s4.MATCH,"|")#][#ArrayToList(a1,"|")#][#ArrayToList(a2,"|")#][#s5#][#s6#]
</cfoutput>
