<cfset dir = "">

<cfset r1 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(r1, "red")>
<cfset ImageDrawRect(r1, 3, 3, 10, 8, false)>
<cfset ImageWrite(r1, dir & "dr_r1.png", true)>

<cfset r2 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(r2, "red")>
<cfset ImageDrawRect(r2, 3, 3, 10, 8, true)>
<cfset ImageWrite(r2, dir & "dr_r2.png", true)>

<cfset p1 = ImageNew("", 10, 10, "rgb")>
<cfset ImageSetDrawingColor(p1, "red")>
<cfset ImageDrawPoint(p1, 5, 5)>
<cfset ImageDrawPoint(p1, 0, 0)>
<cfset ImageDrawPoint(p1, 9, 9)>
<cfset ImageWrite(p1, dir & "dr_p1.png", true)>

<cfset l1 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(l1, "red")>
<cfset ImageDrawLine(l1, 15, 0, 15, 29)>
<cfset ImageWrite(l1, dir & "dr_l1.png", true)>

<cfset l2 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(l2, "red")>
<cfset ImageDrawLine(l2, 0, 5, 29, 5)>
<cfset ImageWrite(l2, dir & "dr_l2.png", true)>

<cfset l3 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(l3, "blue")>
<cfset ImageDrawLine(l3, 2, 2, 17, 17)>
<cfset ImageWrite(l3, dir & "dr_l3.png", true)>

<cfset v1 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(v1, "blue")>
<cfset ImageDrawOval(v1, 3, 3, 12, 12, false)>
<cfset ImageWrite(v1, dir & "dr_v1.png", true)>

<cfset v2 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(v2, "blue")>
<cfset ImageDrawOval(v2, 3, 3, 12, 12, true)>
<cfset ImageWrite(v2, dir & "dr_v2.png", true)>

<cfset a1 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(a1, "red")>
<cfset ImageDrawArc(a1, 3, 3, 20, 20, 0, 90, false)>
<cfset ImageWrite(a1, dir & "dr_a1.png", true)>

<cfset a2 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(a2, "red")>
<cfset ImageDrawArc(a2, 3, 3, 20, 20, 0, -90, true)>
<cfset ImageWrite(a2, dir & "dr_a2.png", true)>

<cfset rr1 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(rr1, "red")>
<cfset ImageDrawRoundRect(rr1, 3, 3, 12, 12, 4, 4, true)>
<cfset ImageWrite(rr1, dir & "dr_rr1.png", true)>

<cfset rr2 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(rr2, "red")>
<cfset ImageDrawRoundRect(rr2, 3, 3, 12, 12, 4, 4, false)>
<cfset ImageWrite(rr2, dir & "dr_rr2.png", true)>

<cfset b1 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(b1, "red")>
<cfset ImageDrawBeveledRect(b1, 2, 2, 12, 12, true, true)>
<cfset ImageWrite(b1, dir & "dr_b1.png", true)>

<cfset b2 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(b2, "red")>
<cfset ImageDrawBeveledRect(b2, 2, 2, 12, 12, false, true)>
<cfset ImageWrite(b2, dir & "dr_b2.png", true)>

<cfset b3 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(b3, "red")>
<cfset ImageDrawBeveledRect(b3, 2, 2, 12, 12, true, false)>
<cfset ImageWrite(b3, dir & "dr_b3.png", true)>

<cfset lg1 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(lg1, "red")>
<cfset ImageDrawLines(lg1, [2, 10, 26, 18], [2, 26, 18, 4], true, false)>
<cfset ImageWrite(lg1, dir & "dr_lg1.png", true)>

<cfset lg2 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(lg2, "red")>
<cfset ImageDrawLines(lg2, [2, 10, 26, 18], [2, 26, 18, 4], true, true)>
<cfset ImageWrite(lg2, dir & "dr_lg2.png", true)>

<cfset lg3 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(lg3, "red")>
<cfset ImageDrawLines(lg3, [2, 10, 26, 18], [2, 26, 18, 4], false, false)>
<cfset ImageWrite(lg3, dir & "dr_lg3.png", true)>

<cfset c1 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(c1, "red")>
<cfset ImageDrawCubicCurve(c1, 2, 2, 26, 2, 2, 26, 26, 26)>
<cfset ImageWrite(c1, dir & "dr_c1.png", true)>

<cfset c2 = ImageNew("", 30, 30, "rgb")>
<cfset ImageSetDrawingColor(c2, "red")>
<cfset ImageDrawQuadraticCurve(c2, 2, 26, 26, 26, 15, 2)>
<cfset ImageWrite(c2, dir & "dr_c2.png", true)>

<cfset cl1 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetBackgroundColor(cl1, "red")>
<cfset ImageClearRect(cl1, 3, 3, 8, 8)>
<cfset ImageWrite(cl1, dir & "dr_cl1.png", true)>

<cfset t1 = ImageNew("", 10, 10, "rgb")>
<cfset ImageSetDrawingColor(t1, "blue")>
<cfset ImageSetDrawingTransparency(t1, 50)>
<cfset ImageDrawRect(t1, 0, 0, 10, 10, true)>
<cfset ImageWrite(t1, dir & "dr_t1.png", true)>

<cfset x1 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(x1, "blue")>
<cfset ImageXORDrawingMode(x1, "white")>
<cfset ImageDrawRect(x1, 3, 3, 8, 8, true)>
<cfset ImageWrite(x1, dir & "dr_x1.png", true)>

<cfset sw1 = ImageNew("", 20, 20, "rgb")>
<cfset ImageSetDrawingColor(sw1, "blue")>
<cfset ImageSetDrawingStroke(sw1, {width: 3})>
<cfset ImageDrawLine(sw1, 2, 2, 17, 17)>
<cfset ImageWrite(sw1, dir & "dr_sw1.png", true)>

<cfoutput>written</cfoutput>
