<cfhtmlhead text="<meta name='a' content='1'>">
<cfhtmlhead text="#Chr(10)#<title>My Title</title>">
<cfset h = "dyn">
<cfhtmlhead text="<meta name='dyn' content='#h#'>">
<cfoutput>BODY</cfoutput>
