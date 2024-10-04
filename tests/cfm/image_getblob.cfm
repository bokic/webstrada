<cfset img = ImageNew("", 6, 4, "rgb") />
<cfset ImageWrite(img, "img_blob_src.png", true) />
<cfset src = ImageRead("img_blob_src.png") />
<cfset blob = ImageGetBlob(src) />
<cfoutput>#IsBinary(blob)#|#ImageGetWidth(ImageReadBase64(ToBase64(blob)))#x#ImageGetHeight(ImageReadBase64(ToBase64(blob)))#</cfoutput>
<cfset blank = ImageNew("", 3, 3, "rgb") />
<cftry>
  <cfset b2 = ImageGetBlob(blank) />
  <cfoutput>|NOERR</cfoutput>
  <cfcatch type="any"><cfoutput>|[#cfcatch.type#] [#cfcatch.message#] [#cfcatch.detail#]</cfoutput></cfcatch>
</cftry>
