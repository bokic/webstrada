<!--- <cfimage> tag actions verified byte-for-byte against CF 2025 (RDS host).
All writes use overwrite="true" so re-runs cannot collide with a leftover file.
--->
<cfset sep = "~">
<cfset im = ImageNew("", 8, 5, "rgb")>
<cfset ImageWrite(im, "cfimage_src.png", true)>
<cfimage action="read" source="cfimage_src.png" name="r1">
<cfoutput>#sep#READ:#ImageGetWidth(r1)#x#ImageGetHeight(r1)#</cfoutput>
<cfimage action="read" source="#r1#" name="r2">
<cfoutput>#sep#READ_COPY:#ImageGetWidth(r2)#x#ImageGetHeight(r2)#</cfoutput>
<cfimage action="write" source="#r1#" destination="cfimage_out1.png" overwrite="true">
<cfset ri = ImageRead("cfimage_out1.png")>
<cfoutput>#sep#WRITE:#ImageGetWidth(ri)#x#ImageGetHeight(ri)#</cfoutput>
<cfimage action="resize" source="#r1#" width="16" height="10" destination="cfimage_out2.png" name="r3" overwrite="true">
<cfoutput>#sep#RESIZE:#ImageGetWidth(r3)#x#ImageGetHeight(r3)#</cfoutput>
<cfimage action="resize" source="#r1#" width="200%" height="100%" destination="cfimage_out6.png" name="r6" overwrite="true">
<cfoutput>#sep#RESIZE_PCT:#ImageGetWidth(r6)#x#ImageGetHeight(r6)#</cfoutput>
<cfimage action="border" source="#r1#" thickness="2" color="blue" destination="cfimage_out3.png" name="r4" overwrite="true">
<cfoutput>#sep#BORDER:#ImageGetWidth(r4)#x#ImageGetHeight(r4)#</cfoutput>
<cfimage action="border" source="#r1#" thickness="3" color="FF0000" destination="cfimage_out7.png" name="r7" overwrite="true">
<cfoutput>#sep#BORDER_HEX:#ImageGetWidth(r7)#x#ImageGetHeight(r7)#</cfoutput>
<cfimage action="rotate" source="#r1#" angle="90" destination="cfimage_out4.png" name="r5" overwrite="true">
<cfoutput>#sep#ROTATE:#ImageGetWidth(r5)#x#ImageGetHeight(r5)#</cfoutput>
<cfimage action="info" source="#r1#" structname="info1">
<cfoutput>#sep#INFO:#info1.width#x#info1.height#</cfoutput>
<cfimage action="convert" source="#r1#" destination="cfimage_out5.png" name="r8" overwrite="true">
<cfoutput>#sep#CONVERT:#ImageGetWidth(r8)#x#ImageGetHeight(r8)#</cfoutput>
<cfimage source="#r1#" name="r9">
<cfoutput>#sep#READ_DEFAULT:#ImageGetWidth(r9)#x#ImageGetHeight(r9)#</cfoutput>
