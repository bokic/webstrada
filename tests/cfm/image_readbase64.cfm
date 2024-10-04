<cfset img = ImageNew("", 16, 8, "rgb") />
<cfset ImageWrite(img, "img_rb64_src.png", true) />
<cfset src = ImageRead("img_rb64_src.png") />
<cfset b64 = ToBase64(ImageGetBlob(src)) />
<cfset rt = ImageReadBase64(b64) />
<cfoutput>
#ImageGetWidth(rt)#x#ImageGetHeight(rt)#
|#ImageInfo(rt).width#x#ImageInfo(rt).height#
|#SerializeJSON(ImageInfo(rt).colormodel)#
</cfoutput>
<cfset uri = "data:image/png;base64,#b64#" />
<cfset rtu = ImageReadBase64(uri) />
<cfoutput>
|#ImageGetWidth(rtu)#x#ImageGetHeight(rtu)#
</cfoutput>
<cfset argb = ImageNew("", 5, 3, "argb") />
<cfset ImageWrite(argb, "img_rb64_argb.png", true) />
<cfset srca = ImageRead("img_rb64_argb.png") />
<cfset b64a = ToBase64(ImageGetBlob(srca)) />
<cfset rta = ImageReadBase64(b64a) />
<cfoutput>
|#ImageGetWidth(rta)#x#ImageGetHeight(rta)#
|#SerializeJSON(ImageInfo(rta).colormodel)#
</cfoutput>
