<cfset x = 5>
<cfoutput>
1:[#IIf(x GT 3, "x", "0")#]
2:[#IIf(x GT 9, "x", "0")#]
3:[#IIf(0, "x", "x+1")#]
4:[#IIf(1, "x", "x+1")#]
5:[#IIf(true, "'str'", "'other'")#]
6:[#IIf(false, "'str'", "'other'")#]
7:[#IIf(x EQ 5, "10/2", "0")#]
</cfoutput>
