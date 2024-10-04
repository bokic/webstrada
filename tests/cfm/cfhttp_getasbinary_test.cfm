<!--- cfhttp getasbinary="no" (default) with a non-text MIME body: CF stores a
      ByteArrayOutputStream — IsBinary NO, Len = byte count, stringify decodes
      the bytes as UTF-8 (was BUGS.md #28) --->
<cfhttp url="http://192.168.100.75:8899/img.png" method="GET" result="r">
</cfhttp>
<cfoutput>
NO:isbin=#IsBinary(r.fileContent)#|len=#Len(r.fileContent)#|mime=#r.mimeType#|first=#mid(r.fileContent,1,4)#
</cfoutput>
<cfhttp url="http://192.168.100.75:8899/img.png" method="GET" getasbinary="yes" result="rb">
</cfhttp>
<cfoutput>
YES:isbin=#IsBinary(rb.fileContent)#|len=#Len(rb.fileContent)#|mime=#rb.mimeType#
</cfoutput>
<cfhttp url="http://192.168.100.75:8899/img.png" method="GET" getasbinary="auto" result="ra">
</cfhttp>
<cfoutput>
AUTO:isbin=#IsBinary(ra.fileContent)#|len=#Len(ra.fileContent)#|mime=#ra.mimeType#
</cfoutput>
<cfhttp url="http://192.168.100.75:8899/hello.txt" method="GET" result="rt">
</cfhttp>
<cfoutput>
TXT:isbin=#IsBinary(rt.fileContent)#|len=#Len(rt.fileContent)#|fc=#rt.fileContent#
</cfoutput>
