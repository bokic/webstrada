<cfset dir = "">

<!--- EXACT: cumulative translates then line --->
<cfset t1 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(t1, "red")>
<cfset ImageTranslateDrawingAxis(t1, 3, 4)>
<cfset ImageTranslateDrawingAxis(t1, 2, 1)>
<cfset ImageDrawLine(t1, 0, 0, 5, 0)>
<cfset ImageWrite(t1, dir & "ax_t1.png", true)>

<!--- rotate 90 deg about origin: line goes off-image (empty) --->
<cfset r1 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(r1, "red")>
<cfset ImageRotateDrawingAxis(r1, 90, 0, 0)>
<cfset ImageDrawLine(r1, 0, 10, 0, 20)>
<cfset ImageWrite(r1, dir & "ax_r1.png", true)>

<!--- rotate 90 about center --->
<cfset r2 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(r2, "red")>
<cfset ImageRotateDrawingAxis(r2, 90, 15, 15)>
<cfset ImageDrawLine(r2, 0, 15, 30, 15)>
<cfset ImageWrite(r2, dir & "ax_r2.png", true)>

<!--- shear x as function of y --->
<cfset s1 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(s1, "red")>
<cfset ImageShearDrawingAxis(s1, 0.5, 0)>
<cfset ImageDrawLine(s1, 0, 0, 0, 10)>
<cfset ImageWrite(s1, dir & "ax_s1.png", true)>

<!--- shear y as function of x --->
<cfset s2 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(s2, "red")>
<cfset ImageShearDrawingAxis(s2, 0, 0.5)>
<cfset ImageDrawLine(s2, 0, 0, 10, 0)>
<cfset ImageWrite(s2, dir & "ax_s2.png", true)>

<!--- translate then rotate combined --->
<cfset m1 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(m1, "red")>
<cfset ImageTranslateDrawingAxis(m1, 15, 15)>
<cfset ImageRotateDrawingAxis(m1, 90, 0, 0)>
<cfset ImageDrawLine(m1, -10, 0, 0, 0)>
<cfset ImageWrite(m1, dir & "ax_m1.png", true)>

<!--- EXACT: filled rect under pure translate --->
<cfset x1 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(x1, "red")>
<cfset ImageTranslateDrawingAxis(x1, 5, 5)>
<cfset ImageDrawRect(x1, 0, 0, 10, 10, true)>
<cfset ImageWrite(x1, dir & "ax_x1.png", true)>

<!--- outline rect under 90-deg rotation --->
<cfset x2 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(x2, "red")>
<cfset ImageRotateDrawingAxis(x2, 90, 15, 15)>
<cfset ImageDrawRect(x2, 10, 10, 10, 10, false)>
<cfset ImageWrite(x2, dir & "ax_x2.png", true)>

<!--- EXACT: point under pure translate --->
<cfset p1 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(p1, "red")>
<cfset ImageTranslateDrawingAxis(p1, 5, 5)>
<cfset ImageDrawPoint(p1, 3, 3)>
<cfset ImageWrite(p1, dir & "ax_p1.png", true)>

<!--- EXACT: clearrect under pure translate --->
<cfset clr = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(clr, "red")>
<cfset ImageDrawRect(clr, 0, 0, 29, 29, true)>
<cfset ImageSetBackgroundColor(clr, "white")>
<cfset ImageTranslateDrawingAxis(clr, 5, 5)>
<cfset ImageClearRect(clr, 0, 0, 10, 10)>
<cfset ImageWrite(clr, dir & "ax_clr.png", true)>
