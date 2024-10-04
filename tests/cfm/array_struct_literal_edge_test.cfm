<cfset s = {a=5,b=6}><cfoutput>EQ=#s.a#|#s.b#</cfoutput>
<cfset s2 = { a : 1 , b : 2 }><cfoutput>SP=#s2.a#|#s2.b#</cfoutput>
<cfset deep = [1, [2, [3]]]><cfoutput>DEEP=#deep[2][2][1]#</cfoutput>
<cfset mos = [{x:1},{x:2}]><cfoutput>MOS=#mos[2].x#|#mos[1].x#</cfoutput>
<cfscript>a = [1,2,3]; writeOutput("SCR=#a[2]#");</cfscript>
<cfscript>st = {a:1,b:2}; writeOutput("|ST=#st.a#");</cfscript>
<cfscript>writeOutput("|FA=#ArrayLen([1,2])#");</cfscript>
<cfset arr3 = [10,20,30]><cfoutput>#arr3[ArrayLen(arr3)]#</cfoutput>
<cfset emptyStruct = {}><cfoutput>[#StructCount(emptyStruct)#]</cfoutput>
<cfset mixed = [1, "a,b", [2,3]]><cfoutput>#mixed[2]#|#mixed[3][1]#|#mixed[3][2]#</cfoutput>
<cfset s3 = {a: 1 + 2}><cfoutput>SA=#s3.a#</cfoutput>
<cfset x = 5><cfset s4 = {a: x * 2}><cfoutput>SV=#s4.a#</cfoutput>
<cfset a4 = [1 + 1, "s", true, 4.5]><cfoutput>AM=#a4[1]#|#a4[2]#|#a4[3]#|#a4[4]#</cfoutput>
<cfset s5 = {name:"bob", age:30}><cfoutput>QK=#s5.name#|#s5.age#</cfoutput>
<cfset m5 = [["a","b"],["c","d"]]><cfoutput>M5=#m5[1][2]#|#m5[2][1]#</cfoutput>
<cfset s6 = {a:{b:{c:42}}}><cfoutput>S6=#s6.a.b.c#</cfoutput>
<cfoutput>NEST=#ArrayLen([[1,2],[3,4],[5,6]])#</cfoutput>
