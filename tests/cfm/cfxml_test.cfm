<cfxml variable="xdoc"><root><child id="1">text</child></root></cfxml><cfoutput>IsXmlDoc:#IsXmlDoc(xdoc)#|RootName:#xdoc.XmlRoot.XmlName#|ChildName:#xdoc.XmlRoot.XmlChildren[1].XmlName#|ChildText:#xdoc.XmlRoot.XmlChildren[1].XmlText#|ChildAttr:#xdoc.XmlRoot.XmlChildren[1].XmlAttributes.id#|</cfoutput>
<cfset varName = "dynDoc" /><cfxml variable="#varName#"><a><b>hello</b></a></cfxml><cfoutput>DynDoc:#IsXmlDoc(dynDoc)#|DynRoot:#dynDoc.XmlRoot.XmlName#|</cfoutput>
<cfxml variable="caseDoc" casesensitive="true"><RooT><CHILD>val</CHILD></RooT></cfxml><cfoutput>CaseRoot:#caseDoc.XmlRoot.XmlName#|CaseChild:#caseDoc.XmlRoot.XmlChildren[1].XmlName#|</cfoutput>
<cfxml variable="ciDoc"><Root attr="val"><Child>text</Child></Root></cfxml><cfoutput>CI_Upper:#ciDoc.ROOT.XmlName#|CI_Lower:#ciDoc.root.XmlName#|CI_Attr:#ciDoc.ROOT.XmlAttributes.attr#|CI_Child:#ciDoc.ROOT.CHILD.XmlText#|CI_ChildB:#ciDoc.XmlRoot.Child.XmlText#|</cfoutput>
<cfxml variable="csDoc" casesensitive="true"><Root attr="val"><Child>text</Child></Root></cfxml><cfoutput>CS_Root:#csDoc["Root"].XmlName#|CS_Child:#csDoc["Root"]["Child"].XmlText#|CS_Attr:#csDoc["Root"].XmlAttributes.attr#|</cfoutput>
<cfxml variable="wsDoc">

<outer>
  <inner>data</inner>
</outer>

</cfxml><cfoutput>WsRoot:#wsDoc.XmlRoot.XmlName#|WsChild:#wsDoc.XmlRoot.XmlChildren[1].XmlName#|WsChildText:#wsDoc.XmlRoot.XmlChildren[1].XmlText#|</cfoutput>
<cfset pname = "prod" /><cfxml variable="plainVar"><root><a>#pname#</a></root></cfxml><cfoutput>InterpPlain:#plainVar.XmlRoot.XmlChildren[1].XmlText#|</cfoutput>
<cfxml variable="withCfoutput"><root><c><cfoutput>#pname#</cfoutput></c></root></cfxml><cfoutput>InterpCfoutput:#withCfoutput.XmlRoot.XmlChildren[1].XmlText#|</cfoutput>
<cfxml variable="selfdoc"><root/></cfxml><cfoutput>SelfDoc:#IsXmlDoc(selfdoc)#|SelfRoot:#selfdoc.XmlRoot.XmlName#|</cfoutput>
<cfxml variable="multiline"><root>
<a>1</a>
</root>
</cfxml><cfoutput>MLText:#multiline.XmlRoot.XmlChildren[1].XmlText#|</cfoutput>
<cfxml variable="sib"><root><a>1</a><a>2</a></root></cfxml><cfoutput>SiblingCount:#sib.XmlRoot.XmlChildren.len()#|Sibling1:#sib.XmlRoot.XmlChildren[1].XmlText#|Sibling2:#sib.XmlRoot.XmlChildren[2].XmlText#|</cfoutput>
<cfxml variable="mixed"><root>pre<a>1</a>mid<b>2</b>post</root></cfxml><cfoutput>MixedText:#mixed.XmlRoot.XmlText#|MixedKids:#mixed.XmlRoot.XmlChildren.len()#|MixedA:#mixed.XmlRoot.XmlChildren[1].XmlText#|MixedNodes:#mixed.XmlRoot.XmlNodes.len()#|</cfoutput>
<cfxml variable="same"><root><Child>1</Child><Child>2</Child></root></cfxml><cfoutput>SameArr:#isArray(same.XmlRoot.CHILD)#|SameLen:#same.XmlRoot.CHILD.len()#|Same1:#same.XmlRoot.CHILD[1].XmlText#|Same2:#same.XmlRoot.CHILD[2].XmlText#|</cfoutput>
<cfxml variable="single"><root><Child>x</Child></root></cfxml><cfoutput>SingleArr:#isArray(single.XmlRoot.CHILD)#|SingleText:#single.XmlRoot.CHILD.XmlText#|</cfoutput>
