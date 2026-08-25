<cfimport prefix="mytag" taglib="customtags">
<cfoutput>[A]</cfoutput>
<mytag:selfcheck />
<cfoutput>[B]</cfoutput>
<mytag:selfcheck>body</mytag:selfcheck>
<cfoutput>[C]</cfoutput>
