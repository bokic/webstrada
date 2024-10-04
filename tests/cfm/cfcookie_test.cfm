<cfcookie name="foo" value="bar">
<cfcookie name="baz" value="hello world">
<cfcookie name="sp" value="a b" encodevalue="no">
<cfcookie name="pc" value="x" preserveCase="yes">
<cfcookie name="empty" value="">
<cfcookie name="pc2" value="Y" preserveCase="no">
<cfoutput>#cookie.foo#|#cookie.baz#|#cookie.sp#|#cookie.pc#|#cookie.empty#|#cookie.pc2#</cfoutput>
