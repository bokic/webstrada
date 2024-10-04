<cfoutput>
A:#HTMLCodeFormat("a & b < c > d")#
B:#HTMLCodeFormat("q &quot; w "" e ' r")#
C:#HTMLCodeFormat("single ' quote")#
D:#HTMLCodeFormat("cr" & Chr(13) & "dropped")#
E:#HTMLCodeFormat("nl" & Chr(10) & "kept")#
F:#HTMLCodeFormat("crlf" & Chr(13) & Chr(10) & "x")#
G:#HTMLCodeFormat("")#
H:#HTMLCodeFormat("tab	here")#
I:#HTMLCodeFormat("100% sure")#
J:#HTMLCodeFormat("a&amp;b")#
K:#HTMLCodeFormat("x&amp;amp;y")#
L:#HTMLCodeFormat("a &lt;b&gt; c")#
M:#HTMLCodeFormat("it's")#
N:#HTMLCodeFormat("back\slash")#
O:#HTMLCodeFormat("a & b < c > d", 2)#
P:#HTMLCodeFormat("a & b < c > d", -1)#
Q:#HTMLCodeFormat("&copy; &reg; &trade;")#
R:#HTMLCodeFormat("a" & Chr(34) & "b" & Chr(34) & "c")#
S:#HTMLCodeFormat("a < b & c > d" & Chr(10) & "next")#
<cfscript>
c1 = HTMLCodeFormat("a & b < c > d");
c2 = HTMLCodeFormat("a &amp; b");
c3 = HTMLCodeFormat("cr" & Chr(13) & "x");
writeOutput("C1:#c1#|C2:#c2#|C3:#c3#");
</cfscript>
</cfoutput>
