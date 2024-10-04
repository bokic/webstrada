<cfset a = 99 />
<cfxml variable="withCfset"><root><b><cfset inner = "setval" />#inner#</b></root></cfxml><cfoutput>InterpCfset:#withCfset.XmlRoot.XmlChildren[1].XmlText#|</cfoutput>
<cfxml variable="w1"><root><cfset a=1/>    <cfset b=2/><c>x</c></root></cfxml><cfoutput>W_Text:#w1.XmlRoot.XmlText#|W_Kids:#w1.XmlRoot.XmlChildren.len()#|W_C:#w1.XmlRoot.XmlChildren[1].XmlName#|</cfoutput>
<cfxml variable="w2"><root><cfset a=1/>
<cfset b=2/><c>x</c></root></cfxml><cfoutput>W2_Text:#w2.XmlRoot.XmlText#|W2_Kids:#w2.XmlRoot.XmlChildren.len()#|</cfoutput>
<cfset x = "outer" />
<cfxml variable="nested"><root><cfset x = "inner" />#x#</root></cfxml><cfoutput>NestedX:#x#|</cfoutput>
