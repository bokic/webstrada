A
<cflog text="hello world">
B
<cflog text="warn msg" type="warning">
C
<cflog text="err msg" type="error">
D
<cflog text="fatal msg" type="fatal">
E
<cflog text="info msg" type="information">
F
<cflog text="weird type" type="bogus">
G
<cflog text="custom file" file="WebStrada_verify_test">
H
<cflog text="scheduler log" log="scheduler">
I
<cflog text="app off" application="false">
J
<cflog text="app on" application="true">
K
<cfset x = "dynamic text">
<cflog text="#x#">
L
<cflog text="">
M
<cfapplication name="cflogTestApp">
<cflog text="app col on">
N
<cflog text="body skipped">BODY_SKIPPED</cflog>
O
<cfoutput>END</cfoutput>
