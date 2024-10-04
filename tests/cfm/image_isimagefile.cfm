<cfset img = ImageNew("", 4, 4, "rgb") />
<cfset ImageWrite(img, "img_isf_png.png", true) />
<cfoutput>
#IsImageFile("img_isf_png.png")#
|#IsImageFile("img_isf_missing_zzz.png")#
|#IsImageFile(".")#
</cfoutput>
