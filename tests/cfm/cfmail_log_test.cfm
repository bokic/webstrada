<cfset recipient = "to@example.com" />
<cfmail to="#recipient#" from="from@example.com" subject="Test message" server="smtp.example.com" username="u" password="p">
    body text
    <cfmailparam file="/tmp/example.txt" type="text/plain" />
    <cfmailpart type="html"><b>html body</b></cfmailpart>
</cfmail>
<cfoutput>AFTER</cfoutput>
