<!--- Tier-1: GetContextRoot / IsDebugMode / client vars / GetCSPNonce /
     PreserveSingleQuotes / SetVariable / ObjectEquals (verified vs CF 2025). --->
<cfset pq = "a'b">
<cfoutput>
1:[#GetContextRoot()#]E|2:[#IsDebugMode()#]|3:[#GetClientVariablesList()#]E|4:[#DeleteClientVariable("nope")#]|5:[#Len(GetCSPNonce())#]|6:[#PreserveSingleQuotes(pq)#]|7:[#SetVariable("pv1", "setval")#][#PV1#]|8:[#SetVariable("a.b.c", "deep")#][#A.B.C#]|9:[#SetVariable("form.x", "f")#][#form.x#]|10:[#ObjectEquals("abc","abc")#]|11:[#ObjectEquals("abc","abd")#]|12:[#ObjectEquals(1,1)#]|13:[#ObjectEquals(1,"1")#]|14:[#ObjectEquals(1,1.0)#]|15:[#ObjectEquals([1,2],[1,2])#]|16:[#ObjectEquals([1,2],[1,3])#]|17:[#ObjectEquals({a:1},{a:1})#]|18:[#ObjectEquals({a:1},{a:2})#]|19:[#ObjectEquals(structnew(),structnew())#]|20:[#ObjectEquals([],[])#]|21:[#ObjectEquals({a:[1,{b:2}]},{a:[1,{b:2}]})#]
</cfoutput>
