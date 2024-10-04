<cfset n = "" /><cfset m = "x" /><cfoutput>
1:#("X=" & n NEQ "")#|
2:#("X=" & n EQ "")#|
3:#("X=" & m EQ "")#|
4:#("X=" & m NEQ "")#|
5:#("" EQ "")#|
6:#(n EQ "")#|
7:#(m EQ "")#|
8:#(n EQ m)#|
9:#("" NEQ "")#|
10:#("" & n EQ "")#|
11:#(n & "" EQ "")#|
12:#(m NEQ n)#|
13:#(m & n NEQ "x")#|
14:#("X=" & n EQ "")#|
15:#(1 NEQ 2)#|
16:#("a" & 1 NEQ 2)#|
17:#("a" NEQ 2)#|
18:#("a" EQ 2)#|</cfoutput><cfset n = ""><cfscript>
writeOutput("S1:" & ("X=" & n NEQ "") & "|");
writeOutput("S2:" & ("X=" & n EQ "") & "|");
writeOutput("S3:" & ("X=" & m EQ "") & "|");
writeOutput("S4:" & ("X=" & m NEQ "") & "|");
writeOutput("S5:" & ("" EQ "") & "|");
writeOutput("S6:" & (n EQ "") & "|");
writeOutput("S7:" & (m EQ "") & "|");
writeOutput("S8:" & (n EQ m) & "|");
writeOutput("S9:" & ("" NEQ "") & "|");
writeOutput("S10:" & ("" & n EQ "") & "|");
writeOutput("S11:" & (n & "" EQ "") & "|");
writeOutput("S12:" & (m NEQ n) & "|");
writeOutput("S13:" & (m & n NEQ "x") & "|");
writeOutput("S14:" & ("X=" & n EQ "") & "|");
writeOutput("S15:" & (1 NEQ 2) & "|");
writeOutput("S16:" & ("a" & 1 NEQ 2) & "|");
writeOutput("S17:" & ("a" NEQ 2) & "|");
writeOutput("S18:" & ("a" EQ 2) & "|");
</cfscript>
