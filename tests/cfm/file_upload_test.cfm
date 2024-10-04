<cfset dest = GetTempDirectory() & "upload_test_dir">
<cfset hasFile = StructKeyExists(form, "file1")>
<cfoutput>upload_present:#hasFile#</cfoutput>
<cfif hasFile>
  <cfset up = FileUpload(dest, "file1")>
  <cfoutput>
serverfile:#up.SERVERFILE#
serverfilename:#up.SERVERFILENAME#
serverfileext:#up.SERVERFILEEXT#
clientfile:#up.CLIENTFILE#
clientfilename:#up.CLIENTFILENAME#
clientfileext:#up.CLIENTFILEEXT#
contenttype:#up.CONTENTTYPE#
contentsubtype:#up.CONTENTSUBTYPE#
filesize:#up.FILESIZE#
filewassaved:#up.FILEWASSAVED#
fileexisted:#up.FILEEXISTED#
filewasoverwritten:#up.FILEWASOVERWRITTEN#
filewasrenamed:#up.FILEWASRENAMED#
attemptedserverfile:#up.ATTEMPTEDSERVERFILE#
  </cfoutput>
  <cfset ups = FileUploadAll(GetTempDirectory() & "upload_all_dir", "*", "makeunique", true, false, "uploadErrors", "*")>
  <cfset firstFile = ups[1].SERVERFILE>
  <cfoutput>
uploadall_count:#ArrayLen(ups)#
uploadall_first:#firstFile#
  </cfoutput>
</cfif>
