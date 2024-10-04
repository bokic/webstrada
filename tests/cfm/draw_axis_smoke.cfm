<cfset sep = "~">

<!--- ImageTranslateDrawingAxis: valid 3-arg call, then draw shifted --->
<cftry>
	<cfset im = ImageNew("", 30, 30, "rgb")>
	<cfset ImageTranslateDrawingAxis(im, 3, 4)>
	<cfset ImageDrawRect(im, 0, 0, 5, 5, true)>
	<cfoutput>#sep#t_ok</cfoutput>
<cfcatch type="any"><cfoutput>#sep#t_err[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>

<!--- cumulative translates --->
<cftry>
	<cfset ImageTranslateDrawingAxis(im, 2, 1)>
	<cfset ImageDrawLine(im, 0, 0, 5, 0)>
	<cfoutput>#sep#tc_ok</cfoutput>
<cfcatch type="any"><cfoutput>#sep#tc_err[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>

<!--- ImageRotateDrawingAxis: 2-arg and 4-arg forms --->
<cftry>
	<cfset r = ImageNew("", 30, 30, "rgb")>
	<cfset ImageRotateDrawingAxis(r, 90)>
	<cfset ImageDrawLine(r, 0, 10, 0, 20)>
	<cfoutput>#sep#r2_ok</cfoutput>
<cfcatch type="any"><cfoutput>#sep#r2_err[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>

<cftry>
	<cfset r2 = ImageNew("", 30, 30, "rgb")>
	<cfset ImageRotateDrawingAxis(r2, 90, 15, 15)>
	<cfset ImageDrawLine(r2, 0, 15, 30, 15)>
	<cfoutput>#sep#r4_ok</cfoutput>
<cfcatch type="any"><cfoutput>#sep#r4_err[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>

<!--- ImageShearDrawingAxis: valid 3-arg call --->
<cftry>
	<cfset s = ImageNew("", 30, 30, "rgb")>
	<cfset ImageShearDrawingAxis(s, 0.5, 0.25)>
	<cfset ImageDrawLine(s, 0, 0, 0, 10)>
	<cfoutput>#sep#s_ok</cfoutput>
<cfcatch type="any"><cfoutput>#sep#s_err[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>

<!--- translate + rotate combined --->
<cftry>
	<cfset m = ImageNew("", 30, 30, "rgb")>
	<cfset ImageTranslateDrawingAxis(m, 15, 15)>
	<cfset ImageRotateDrawingAxis(m, 90, 0, 0)>
	<cfset ImageDrawLine(m, -10, 0, 0, 0)>
	<cfoutput>#sep#m_ok</cfoutput>
<cfcatch type="any"><cfoutput>#sep#m_err[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>

<!--- clearrect honors the axis transform --->
<cftry>
	<cfset c = ImageNew("", 30, 30, "rgb")>
	<cfset ImageDrawRect(c, 0, 0, 29, 29, true)>
	<cfset ImageSetBackgroundColor(c, "white")>
	<cfset ImageTranslateDrawingAxis(c, 5, 5)>
	<cfset ImageClearRect(c, 0, 0, 10, 10)>
	<cfoutput>#sep#clr_ok</cfoutput>
<cfcatch type="any"><cfoutput>#sep#clr_err[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>

<!--- point under translate --->
<cftry>
	<cfset p = ImageNew("", 20, 20, "rgb")>
	<cfset ImageTranslateDrawingAxis(p, 5, 5)>
	<cfset ImageDrawPoint(p, 3, 3)>
	<cfoutput>#sep#p_ok</cfoutput>
<cfcatch type="any"><cfoutput>#sep#p_err[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
