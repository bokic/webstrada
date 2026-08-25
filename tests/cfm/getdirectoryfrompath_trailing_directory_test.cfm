<cfset directory_path = GetDirectoryFromPath("/a/b/")>
<cfset file_path = GetDirectoryFromPath("/a/b/file.cfm")>
<cfset root_path = GetDirectoryFromPath("/")>
<cfoutput>directory:#directory_path#|file:#file_path#|root:#root_path#</cfoutput>
