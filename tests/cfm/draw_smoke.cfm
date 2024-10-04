<cfset im = ImageNew("", 30, 30, "rgb")>
<cfset sep = "~">

<cftry><cfset ImageSetAntialiasing(im, "on")><cfoutput>#sep#AAON</cfoutput><cfcatch type="any"><cfoutput>#sep#AAON_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageSetBackgroundColor(im, "red")><cfoutput>#sep#BG</cfoutput><cfcatch type="any"><cfoutput>#sep#BG_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageSetDrawingColor(im, "FF0000")><cfoutput>#sep#DC</cfoutput><cfcatch type="any"><cfoutput>#sep#DC_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageSetDrawingStroke(im, {width: 2})><cfoutput>#sep#STK</cfoutput><cfcatch type="any"><cfoutput>#sep#STK_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageSetDrawingTransparency(im, 0)><cfoutput>#sep#TR</cfoutput><cfcatch type="any"><cfoutput>#sep#TR_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageClearRect(im, 2, 2, 5, 5)><cfoutput>#sep#CLR</cfoutput><cfcatch type="any"><cfoutput>#sep#CLR_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageDrawArc(im, 3, 3, 20, 20, 0, 90, false)><cfoutput>#sep#ARC</cfoutput><cfcatch type="any"><cfoutput>#sep#ARC_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageDrawBeveledRect(im, 2, 2, 12, 12, true, false)><cfoutput>#sep#BEV</cfoutput><cfcatch type="any"><cfoutput>#sep#BEV_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageDrawCubicCurve(im, 2, 2, 26, 2, 2, 26, 26, 26)><cfoutput>#sep#CUB</cfoutput><cfcatch type="any"><cfoutput>#sep#CUB_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageDrawLine(im, 2, 2, 26, 26)><cfoutput>#sep#LIN</cfoutput><cfcatch type="any"><cfoutput>#sep#LIN_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageDrawLines(im, [2, 10, 26, 18], [2, 26, 18, 4], true, false)><cfoutput>#sep#LNS</cfoutput><cfcatch type="any"><cfoutput>#sep#LNS_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageDrawOval(im, 3, 3, 20, 20, false)><cfoutput>#sep#OVL</cfoutput><cfcatch type="any"><cfoutput>#sep#OVL_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageDrawPoint(im, 15, 15)><cfoutput>#sep#PNT</cfoutput><cfcatch type="any"><cfoutput>#sep#PNT_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageDrawQuadraticCurve(im, 2, 26, 26, 26, 15, 2)><cfoutput>#sep#QDR</cfoutput><cfcatch type="any"><cfoutput>#sep#QDR_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageDrawRect(im, 2, 2, 10, 8, false)><cfoutput>#sep#RCT</cfoutput><cfcatch type="any"><cfoutput>#sep#RCT_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageDrawRoundRect(im, 3, 3, 12, 12, 4, 4, true)><cfoutput>#sep#RND</cfoutput><cfcatch type="any"><cfoutput>#sep#RND_E</cfoutput></cfcatch></cftry>
<cftry><cfset ImageXORDrawingMode(im, "white")><cfoutput>#sep#XOR</cfoutput><cfcatch type="any"><cfoutput>#sep#XOR_E</cfoutput></cfcatch></cftry>

<cfset ImageSetDrawingTransparency(im, 0)>
<cfset ImageSetDrawingColor(im, "black")>
<cftry><cfset ImageXORDrawingMode(im, "white")><cfset ImageDrawRect(im, 0, 0, 30, 30, true)><cfset ImageXORDrawingMode(im, "red")><cfset ImageDrawRect(im, 0, 0, 30, 30, true)><cfoutput>#sep#XORRESTORE</cfoutput><cfcatch type="any"><cfoutput>#sep#XORRESTORE_E</cfoutput></cfcatch></cftry>

<cfoutput>#sep#END</cfoutput>
