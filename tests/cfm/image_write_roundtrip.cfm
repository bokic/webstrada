<cfset sep = "|" />
<cfset img = ImageNew("", 8, 4, "rgb") />
<cfset ImageWrite(img, "img_rt_png.png", true) />
<cfset rt = ImageRead("img_rt_png.png") />
<cfoutput>
#ImageGetWidth(rt)#x#ImageGetHeight(rt)#
#sep##SerializeJSON(ImageGetMetadata(rt))#
#sep##ImageInfo(rt).width#x#ImageInfo(rt).height#
#sep##SerializeJSON(ImageInfo(rt).colormodel)#
</cfoutput>
<cfset ImageWrite(img, "img_rt_jpeg.jpg", true) />
<cfset rj = ImageRead("img_rt_jpeg.jpg") />
<cfoutput>
#sep##ImageGetWidth(rj)#x#ImageGetHeight(rj)#
#sep##SerializeJSON(ImageGetMetadata(rj))#
#sep##ImageInfo(rj).width#x#ImageInfo(rj).height#
#sep##SerializeJSON(ImageInfo(rj).colormodel)#
</cfoutput>
<cftry>
  <cfset ImageWrite(img, "img_rt_png.png", 0.5, true) />
  <cfset ImageWrite(img, "img_rt_png.png", 0.5, false) />
  <cfoutput>#sep#NOERR</cfoutput>
  <cfcatch type="any"><cfoutput>#sep#[#cfcatch.type#] [#cfcatch.detail#]</cfoutput></cfcatch>
</cftry>
<cfset ImageWrite(img, "img_rt_q.jpg", 0.35, true) />
<cfset rq = ImageRead("img_rt_q.jpg") />
<cfoutput>
#sep##ImageGetWidth(rq)#x#ImageGetHeight(rq)#
#sep##SerializeJSON(ImageGetMetadata(rq))#
</cfoutput>
