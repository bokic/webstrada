<!--- Tier-1: GetToken (verified against CF 2025 on the RDS host). --->
<cfset s = "a,b,c,d">
<cfset sp = "a b c d">
<cfoutput>
1:[#GetToken(s,2)#]|2:[#GetToken(s,1,";")#]|3:[#GetToken("a,b,c,d",9)#]|4:[#GetToken("",1)#]|5:[#GetToken(sp,3)#]|6:[#GetToken("a.b.c.d",4,".")#]|7:[#GetToken("a,,b",2,",")#]|8:[#GetToken(s,1.9)#]|9:[#GetToken("a,b,c,d",2,";")#]|10:[#GetToken(s,4)#]|11:[#GetToken(sp,1)#]
</cfoutput>
