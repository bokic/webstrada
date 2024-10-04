<!--- cfwddx: serialize/deserialize CFML to/from WDDX, and JS rendering. --->
<cfset s = {title="Hello World", count=5, flag=true, price=3.14, tags=["a","b","c"]}>
<cfwddx action="cfml2wddx" input="#s#" output="wddx">
<cfoutput>#Replace(wddx, "<", "&lt;", "ALL")#</cfoutput>

<!--- number-only input --->
<cfwddx action="cfml2wddx" input="42" output="w1">
<cfoutput>|N1:#Replace(w1, "<", "&lt;", "ALL")#</cfoutput>
<cfwddx action="cfml2wddx" input="2.5" output="w2">
<cfoutput>|N2:#Replace(w2, "<", "&lt;", "ALL")#</cfoutput>
<cfwddx action="cfml2wddx" input="true" output="w3">
<cfoutput>|N3:#Replace(w3, "<", "&lt;", "ALL")#</cfoutput>

<!--- array of mixed values --->
<cfset arr = ["x", 1.5, true, "with <xml> & chars"]>
<cfwddx action="cfml2wddx" input="#arr#" output="w4">
<cfoutput>|ARR:#Replace(w4, "<", "&lt;", "ALL")#</cfoutput>

<!--- usetimezoneinfo=no (timezone-independent output) --->
<cfwddx action="cfml2wddx" input="#CreateDateTime(2020,1,2,3,4,5)#" usetimezoneinfo="no" output="w6">
<cfoutput>|DT2:#Replace(w6, "<", "&lt;", "ALL")#</cfoutput>

<!--- query -> recordset --->
<cfset q = QueryNew("name,age,active", "varchar,integer,bit")>
<cfset QueryAddRow(q)>
<cfset QuerySetCell(q,"name","Alice")>
<cfset QuerySetCell(q,"age",30)>
<cfset QuerySetCell(q,"active",1)>
<cfset QueryAddRow(q)>
<cfset QuerySetCell(q,"name","Bob")>
<cfset QuerySetCell(q,"age",25)>
<cfset QuerySetCell(q,"active",0)>
<cfwddx action="cfml2wddx" input="#q#" output="w7">
<cfoutput>|QRY:#Replace(w7, "<", "&lt;", "ALL")#</cfoutput>

<!--- wddx2cfml: struct round-trip --->
<cfset pkt = "<wddxPacket version='1.0'><header/><data><struct><var name='A'><string>abc</string></var><var name='N'><number>42</number></var><var name='B'><boolean value='true'/></var><var name='ARR'><array length='2'><string>x</string><number>1.5</number></array></var><var name='D'><dateTime>2020-01-02T03:04:05</dateTime></var><var name='NULL'><null/></var></struct></data></wddxPacket>">
<cfwddx action="wddx2cfml" input="#pkt#" output="res">
<cfoutput>|DC:#res.A#:#res.N#:#res.B#:#res.ARR[1]#:#res.ARR[2]#:#StructKeyExists(res,"NULL")#:#IsDate(res.D)#:#res.D#</cfoutput>

<!--- wddx2cfml: recordset --->
<cfset pkt2 = "<wddxPacket version='1.0'><header/><data><recordset rowCount='2' fieldNames='name,age' type='coldfusion.sql.QueryTable'><field name='name'><string>Alice</string><string>Bob</string></field><field name='age'><number>30</number><number>25</number></field></recordset></data></wddxPacket>">
<cfwddx action="wddx2cfml" input="#pkt2#" output="q2">
<cfoutput>|DQ:#IsQuery(q2)#:#q2.recordcount#:#q2.columnlist#:#q2.name[1]#:#q2.age[2]#</cfoutput>

<!--- cfml2js --->
<cfset js = {title="Hello", n=5, d=CreateDateTime(2020,1,2,3,4,5), arr=["a","b"]}>
<cfwddx action="cfml2js" input="#js#" output="j1" toplevelvariable="topvar">
<cfoutput>|JS:#Replace(Replace(j1, "#chr(13)#", "", "ALL"), "#chr(10)#", "\n", "ALL")#</cfoutput>

<!--- wddx2js --->
<cfwddx action="wddx2js" input="#pkt2#" output="j2" toplevelvariable="rs">
<cfoutput>|JQ:#Replace(Replace(j2, "#chr(13)#", "", "ALL"), "#chr(10)#", "\n", "ALL")#</cfoutput>

<!--- invalid action (dynamic -> runtime, catchable) --->
<cfset badact = "bogus">
<cftry>
<cfwddx action="#badact#" input="x">
<cfcatch type="any"><cfoutput>|BADACT:#cfcatch.message#</cfoutput></cfcatch>
</cftry>

