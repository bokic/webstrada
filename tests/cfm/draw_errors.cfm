<cfset sep = "~">
<cfset im = ImageNew("", 4, 4, "rgb")>

<!--- color parsing: valid --->
<cfset okc = ["red", "darkgray", "pink", "FF0000", chr(35) & "FF0000", "aabbcc", "0,0,0", "255,255,255", "10,20,30"]>
<cfloop array="#okc#" index="v">
<cftry>
<cfset ImageSetDrawingColor(im, v)>
<cfoutput>#sep#OK[#v#]</cfoutput>
<cfcatch type="any"><cfoutput>#sep#OK[#v#]E[#cfcatch.type#]</cfoutput></cfcatch>
</cftry>
</cfloop>

<!--- color parsing: required format errors (single value not a name/6hex) --->
<cfset reqc = ["lightblue", "grey", "mediumgray", "255", "F00", "FF000080", "12345", "FF", "0x123", " "]>
<cfloop array="#reqc#" index="v">
<cftry>
<cfset ImageSetDrawingColor(im, v)>
<cfoutput>#sep#RQ[#v#]OK</cfoutput>
<cfcatch type="any"><cfoutput>#sep#RQ[#v#]T[#cfcatch.type#]M[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
</cfloop>

<!--- color parsing: RGB format errors --->
<cfset rgbc = ["1,2,x", "255, 0, 0", "1,2,3,4", "255,0,0,", " 255,0,0", "300,0,0", "256,0,0", "-1,0,0", "0.5,0,0"]>
<cfloop array="#rgbc#" index="v">
<cftry>
<cfset ImageSetDrawingColor(im, v)>
<cfoutput>#sep#RB[#v#]OK</cfoutput>
<cfcatch type="any"><cfoutput>#sep#RB[#v#]T[#cfcatch.type#]M[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
</cfloop>

<!--- color parsing: hex format errors --->
<cfset hexc = [chr(35) & "aabb", chr(35) & chr(35) & "FF0000"]>
<cfloop array="#hexc#" index="v">
<cftry>
<cfset ImageSetDrawingColor(im, v)>
<cfoutput>#sep#HX[#v#]OK</cfoutput>
<cfcatch type="any"><cfoutput>#sep#HX[#v#]T[#cfcatch.type#]M[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
</cfloop>

<!--- color parsing: StringIndexOutOfBounds (2 parts) --->
<cfset sxc = ["0,0", "12,34", "255,0"]>
<cfloop array="#sxc#" index="v">
<cftry>
<cfset ImageSetDrawingColor(im, v)>
<cfoutput>#sep#SX[#v#]OK</cfoutput>
<cfcatch type="any"><cfoutput>#sep#SX[#v#]T[#cfcatch.type#]M[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
</cfloop>

<!--- antialiasing --->
<cfset aav = ["on", "off", "ON", "Off", "true", "false", "no", "1", "0", "yes", "", "ON "]>
<cfloop array="#aav#" index="v">
<cftry>
<cfset ImageSetAntialiasing(im, v)>
<cfoutput>#sep#AA[#v#]OK</cfoutput>
<cfcatch type="any"><cfoutput>#sep#AA[#v#]T[#cfcatch.type#]M[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
</cfloop>

<!--- transparency range --->
<cfset trv = ["0", "100", "50.5", "-1", "101", "150"]>
<cfloop array="#trv#" index="v">
<cftry>
<cfset ImageSetDrawingTransparency(im, v)>
<cfoutput>#sep#TR[#v#]OK</cfoutput>
<cfcatch type="any"><cfoutput>#sep#TR[#v#]T[#cfcatch.type#]M[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
</cfloop>

<!--- stroke width --->
<cfset stv = [{width: 2}, {width: "3"}, {width: -2}, {width: -0.5}, {width: 0}, {width: 1.5}]>
<cfloop array="#stv#" index="v">
<cftry>
<cfset ImageSetDrawingStroke(im, v)>
<cfoutput>#sep#ST[#v.width#]OK</cfoutput>
<cfcatch type="any"><cfoutput>#sep#ST[#v.width#]T[#cfcatch.type#]M[#cfcatch.message#]</cfoutput></cfcatch>
</cftry>
</cfloop>

<cfoutput>#sep#END</cfoutput>
