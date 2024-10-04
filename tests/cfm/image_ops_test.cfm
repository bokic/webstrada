<!--- Image pixel/geometry operations: dimensions, colormodel and error text
verified byte-for-byte against CF 2025 (RDS host). Pixel-level assertions live
in the ImageOpsTest unit tests (tests/tests.cpp). --->
<cfset sep = "~">
<cfset im = ImageNew("", 8, 5, "rgb")>
<cfset ImageSetDrawingColor(im, "red")><cfset ImageDrawRect(im, 1, 1, 3, 2, true)>

<cfset ImageFlip(im, "vertical")><cfoutput>#sep#FLIP_V:#ImageGetWidth(im)#x#ImageGetHeight(im)#</cfoutput>
<cfset ImageFlip(im, "diagonal")><cfoutput>#sep#FLIP_D:#ImageGetWidth(im)#x#ImageGetHeight(im)#</cfoutput>
<cfset ImageFlip(im, "90")><cfoutput>#sep#FLIP_90:#ImageGetWidth(im)#x#ImageGetHeight(im)#</cfoutput>

<cfset im2 = ImageNew("", 8, 5, "rgb")>
<cfset ImageGrayscale(im2)><cfoutput>#sep#GRAY:#ImageGetWidth(im2)#x#ImageGetHeight(im2)#</cfoutput>
<cfset ImageNegative(im2)><cfoutput>#sep#NEG:#ImageGetWidth(im2)#x#ImageGetHeight(im2)#</cfoutput>

<cfset t = ImageMakeColorTransparent(im2, "black")><cfoutput>#sep#TRANSP:#ImageGetWidth(t)#x#ImageGetHeight(t)#</cfoutput>
<cfset tr = ImageMakeTranslucent(im2, 50)><cfoutput>#sep#TRANS:#ImageGetWidth(tr)#x#ImageGetHeight(tr)#</cfoutput>

<cfset im3 = ImageNew("", 8, 5, "rgb")>
<cfset ImageCrop(im3, 2, 1, 4, 3)><cfoutput>#sep#CROP:#ImageGetWidth(im3)#x#ImageGetHeight(im3)#</cfoutput>
<cfset ImageResize(im3, "16", "10", "nearest")><cfoutput>#sep#RESIZE:#ImageGetWidth(im3)#x#ImageGetHeight(im3)#</cfoutput>
<cfset ImageScaleToFit(im3, "8", "8")><cfoutput>#sep#SCALEFIT:#ImageGetWidth(im3)#x#ImageGetHeight(im3)#</cfoutput>
<cfset ImageRotate(im3, 90)><cfoutput>#sep#ROTATE:#ImageGetWidth(im3)#x#ImageGetHeight(im3)#</cfoutput>

<cfset im4 = ImageNew("", 8, 5, "rgb")>
<cfset ImageAddBorder(im4, 2, "blue", "constant")><cfoutput>#sep#BORDER:#ImageGetWidth(im4)#x#ImageGetHeight(im4)#</cfoutput>
<cfset ImageBlur(im4, 3)><cfoutput>#sep#BLUR:#ImageGetWidth(im4)#x#ImageGetHeight(im4)#</cfoutput>
<cfset ImageSharpen(im4, 1.0)><cfoutput>#sep#SHARPEN:#ImageGetWidth(im4)#x#ImageGetHeight(im4)#</cfoutput>

<cfset im5 = ImageNew("", 8, 5, "rgb")>
<cfset c = ImageCopy(im5, 1, 1, 4, 3)><cfoutput>#sep#COPY:#ImageGetWidth(c)#x#ImageGetHeight(c)#</cfoutput>
<cfset ImagePaste(im5, im4, 0, 0)><cfoutput>#sep#PASTE:#ImageGetWidth(im5)#x#ImageGetHeight(im5)#</cfoutput>
<cfset ImageOverlay(im5, im4, "src_over", "0.5")><cfoutput>#sep#OVERLAY:#ImageGetWidth(im5)#x#ImageGetHeight(im5)#</cfoutput>
<cfset ImageShear(im5, 0.5, "horizontal")><cfoutput>#sep#SHEAR:#ImageGetWidth(im5)#x#ImageGetHeight(im5)#</cfoutput>
<cfset ImageTranslate(im5, 2, 3)><cfoutput>#sep#TRANSLATE:#ImageGetWidth(im5)#x#ImageGetHeight(im5)#</cfoutput>

<cftry><cfset ImageBlur(im5, 2)><cfoutput>#sep#NOERR</cfoutput><cfcatch type="any"><cfoutput>#sep#ERR_BLUR:#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfset ImageFlip(im5, "bogus")><cfoutput>#sep#NOERR</cfoutput><cfcatch type="any"><cfoutput>#sep#ERR_FLIP:#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfset ImageSharpen(im5, 5)><cfoutput>#sep#NOERR</cfoutput><cfcatch type="any"><cfoutput>#sep#ERR_SHARP:#cfcatch.message#</cfoutput></cfcatch></cftry>
<cftry><cfset ImageShear(im5, 0.5, "sideways")><cfoutput>#sep#NOERR</cfoutput><cfcatch type="any"><cfoutput>#sep#ERR_SHEAR:#cfcatch.message#</cfoutput></cfcatch></cftry>
