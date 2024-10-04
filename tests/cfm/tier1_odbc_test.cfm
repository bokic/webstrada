<!--- Tier-1: CreateODBCDate / CreateODBCTime (verified against CF 2025). --->
<cfset d = CreateODBCDate(CreateDate(2026,8,8))>
<cfset t = CreateODBCTime(CreateTime(5,30,45))>
<cfset dt = CreateODBCDate(CreateDateTime(2026,8,8,5,30,45))>
<cfoutput>
1:[#d#]|2:[#t#]|3:[#DateFormat(d, "yyyy-mm-dd")#]|4:[#TimeFormat(t, "hh:nn:ss")#]|5:[#IsDate(d)#]|6:[#IsDate(t)#]|7:[#dt#]|8:[#DateAdd("d", 1, d)#]|9:[#Hour(t)#]
</cfoutput>
