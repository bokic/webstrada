<cfset d = GetHttpRequestData()>
<cfset d2 = GetHttpRequestData(false)>
<cfset d3 = GetHttpRequestData(true)>
<cfoutput>
#IsStruct(d)#|#StructKeyExists(d, "headers")#|#StructKeyExists(d, "protocol")#|#StructKeyExists(d, "method")#|#StructKeyExists(d, "content")#
#IsStruct(d.headers)#|#IsSimpleValue(d.method)#|#IsSimpleValue(d.protocol)#|#IsSimpleValue(d.content)#
#StructKeyExists(d2, "content")#|#StructKeyExists(d3, "content")#
#IsStruct(GetHttpRequestData().headers)#
</cfoutput>
