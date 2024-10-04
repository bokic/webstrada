A
<cftrace text="hello" category="cat1" type="information">
B
<cftrace text="before" inline="true">
C
<cftrace>
D
<cftrace abort="true">
E
<cftrace text="body">BODY_EVALUATED</cftrace>
F
<cftrace text="bad type" type="bogus">
G
<cfset cat = "dyncat">
<cftrace text="dyn" category="#cat#">
H
<cftrace text="with var" var="zz_undef_var">
I
<cftrace text="body side effect"><cfset zz2 = 5></cftrace>
<cfoutput>[#zz2#]</cfoutput>
J
<cfoutput>END</cfoutput>
