<cfset a = 5><cfset b = 3><cfset x = "a < b"><cfset y = "p > q"><cfset z = "r <= s"><cfset w = "t >= u">
<cfoutput>W1=[#a LT b#]|W2=[#a GT b#]|W3=[#a LTE a#]|W4=[#a GTE b#]|S1=[#x EQ "a < b"#]|S2=[#y EQ "p > q"#]|S3=[#z EQ "r <= s"#]|S4=[#w EQ "t >= u"#]</cfoutput>
<cfscript>
writeOutput("D1=[" & (1 < 2) & "]|D2=[" & (3 >= 3) & "]|D3=[" & (2 <= 4) & "]|D4=[" & (4 > 1) & "]");
writeOutput("E1=[" & "#x EQ 'a < b'#" & "]|E2=[" & "#z EQ 'r <= s'#" & "]");
</cfscript>
