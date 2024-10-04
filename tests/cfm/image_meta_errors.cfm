<!--- ImageGetEXIFMetadata / ImageGetEXIFTag / ImageGetIPTCMetadata /
ImageGetIPTCTag error paths, verified byte-for-byte against CF 2025. --->
<cfset sep = "~">
<cfset im = ImageNew("", 8, 5, "rgb")>
<cftry><cfoutput>#sep#EXIF_BLANK:#SerializeJSON(ImageGetEXIFMetadata(im))#</cfoutput><cfcatch type="any"><cfoutput>#sep#EXIF_BLANK_ERR:#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cftry><cfoutput>#sep#IPTC_BLANK:#SerializeJSON(ImageGetIPTCMetadata(im))#</cfoutput><cfcatch type="any"><cfoutput>#sep#IPTC_BLANK_ERR:#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cftry><cfoutput>#sep#ETAG_BLANK:#ImageGetEXIFTag(im, "Make")#</cfoutput><cfcatch type="any"><cfoutput>#sep#ETAG_BLANK_ERR:#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cftry><cfoutput>#sep#ITAG_BLANK:#ImageGetIPTCTag(im, "By-line")#</cfoutput><cfcatch type="any"><cfoutput>#sep#ITAG_BLANK_ERR:#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput></cfcatch></cftry>
<cfset pp = ExpandPath("img_meta_p.png")>
<cfset ImageWrite(im, pp, true)>
<cfset ipm = ImageRead(pp)>
<cfoutput>#sep#EXIF_PNG:#SerializeJSON(ImageGetEXIFMetadata(ipm))#</cfoutput>
<cfoutput>#sep#IPTC_PNG:#SerializeJSON(ImageGetIPTCMetadata(ipm))#</cfoutput>
<cfoutput>#sep#ETAG_PNG:#ImageGetEXIFTag(ipm, "Make")#</cfoutput>
<cfoutput>#sep#ITAG_PNG:#ImageGetIPTCTag(ipm, "By-line")#</cfoutput>
