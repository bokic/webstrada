<cfhttp url="http://192.168.100.75:8899/hello.txt" method="GET" result="r">
</cfhttp>
<cfoutput>
#r.statusCode#|#r.mimeType#|#r.text#|#r.charset#|#r.fileContent#
#r.responseHeader.Status_Code#|#r.responseHeader.Http_Version#|#r.responseHeader.Explanation#|#r.responseHeader["Content-Type"]#
</cfoutput>
<cfhttp url="http://192.168.100.75:8899/plain" method="GET" getasbinary="yes" result="r">
</cfhttp>
<cfoutput>
yesbin:#IsBinary(r.fileContent)#|#r.text#|#r.mimeType#|#Len(r.fileContent)#
</cfoutput>
<cfhttp url="http://192.168.100.75:8899/img.png" method="GET" getasbinary="auto" result="r">
</cfhttp>
<cfoutput>
imgauto:#IsBinary(r.fileContent)#|#r.text#|#Len(r.fileContent)#|#r.mimeType#
</cfoutput>
<cfhttp url="http://192.168.100.75:8899/charset.txt" method="GET" result="r">
</cfhttp>
<cfoutput>
charset:#r.charset#|#r.fileContent#
</cfoutput>
<cfhttp url="http://192.168.100.75:8899/error404" method="GET" result="r" throwonerror="no">
</cfhttp>
<cfoutput>
err:#r.statusCode#|#r.fileContent#|#r.mimeType#
</cfoutput>
<cfhttp url="http://192.168.100.75:8899/submit" method="POST" result="r">
<cfhttpparam type="formfield" name="user" value="boris">
<cfhttpparam type="formfield" name="pw" value="a b&c">
</cfhttp>
<cfoutput>
post:#r.statusCode#|#r.fileContent#
</cfoutput>
<cfhttp url="http://192.168.100.75:8899/img.png" method="GET" getasbinary="yes" path="/tmp" file="cfhttp_cfm_dl.png" result="r">
</cfhttp>
<cfoutput>
file:#r.statusCode#|#r.fileContent#|#FileExists("/tmp/cfhttp_cfm_dl.png")#|#GetFileInfo("/tmp/cfhttp_cfm_dl.png").size#
</cfoutput>
<cfhttp url="http://192.168.100.75:8899/redirect" method="GET" result="r">
</cfhttp>
<cfoutput>
redir:#r.statusCode#|#r.fileContent#
</cfoutput>
