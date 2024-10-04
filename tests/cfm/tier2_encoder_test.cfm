<cfset s1 = "&">
<cfset s2 = "<script>alert(1)</script>">
<cfset s3 = 'a"b<c>d&e'>
<cfset s4 = "A-Za-z0-9 !@$%^&*()_+-=[]{}|;:,.?/~`">
<cfset uni = Chr(233) & Chr(937)>
<cfset A = Chr(38)>
<cfset H = Chr(35)>
<cfset c2 = "a" & A & "amp;" & A & "amp;b">
<cfset multi = "a" & A & "amp;lt;b">
<cfoutput>
1:[#EncodeForHTML(s1)#]
2:[#EncodeForHTML(s2)#]
3:[#EncodeForHTML(s3)#]
4:[#EncodeForHTMLAttribute(s3)#]
5:[#EncodeForJavaScript(s3)#]
6:[#EncodeForCSS(s3)#]
7:[#EncodeForXML(s3)#]
8:[#EncodeForXMLAttribute(s3)#]
9:[#EncodeForDN(s1)#]
10:[#EncodeForLDAP(s1)#]
11:[#EncodeForXpath(s1)#]
12:[#EncodeForDN(s2)#]
13:[#EncodeForLDAP(s2)#]
14:[#EncodeForXpath(s2)#]
15:[#EncodeForCSS(s4)#]
16:[#EncodeForJavaScript(s4)#]
17:[#EncodeForDN(s4)#]
18:[#EncodeForLDAP(s4)#]
19:[#EncodeForXpath(s4)#]
20:[#EncodeForHTML(uni)#]
21:[#EncodeForJavaScript(uni)#]
22:[#EncodeForXML(uni)#]
23:[#EncodeForCSS(uni)#]
24:[#EncodeForDN(uni & ' ')#]
25:[#EncodeForLDAP(uni & ' ')#]
26:[#EncodeForDN(' abc ')#]
27:[#EncodeForDN(Chr(35) & 'ab')#]
28:[#DecodeForHTML('a&amp;b&lt;c&gt;d&quot;e')#]
29:[#DecodeForHTML(c2)#]
30:[#DecodeForHTML('&notit')#]
31:[#Canonicalize('a&amp;b', false, false)#]
32:[#Canonicalize(c2, false, false)#]
33:[#Canonicalize(multi, true, true)#]
34:[#Canonicalize(multi, false, false)#]
35:[#Canonicalize('%26', false, false)#]
36:[#Canonicalize('%2526', true, true)#]
</cfoutput>
