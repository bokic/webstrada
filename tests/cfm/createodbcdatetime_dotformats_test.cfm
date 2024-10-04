<!--- CreateODBCDateTime / date parsing of dot-separated dates.
CF accepts YYYY.MM.DD, MM.DD.YYYY and DD.MM.YYYY (preferring MM.DD.YYYY),
with HH:MM or HH:MM:SS times. Verified against CF 2025 on the RDS host. --->
<cfoutput>
1:[#CreateODBCDateTime("2026.05.21 18:22")#]<br>
2:[#CreateODBCDateTime("2026.05.21")#]<br>
3:[#CreateODBCDateTime("21.05.2026 18:22")#]<br>
4:[#CreateODBCDateTime("05.21.2026 18:22")#]<br>
5:[#CreateODBCDateTime("05.06.2026 10:20")#]<br>
6:[#CreateODBCDateTime("13.05.2026 10:20")#]<br>
7:[#CreateODBCDateTime("2026.05.06 10:20")#]<br>
8:[#CreateODBCDateTime("2026.06.05 10:20")#]<br>
9:[#CreateODBCDateTime("2026.05.21 18:22:45")#]<br>
10:[#IsDate("2026.05.21 18:22")#]<br>
11:[#IsDate("2026.05.21")#]<br>
12:[#IsDate("21.05.2026 18:22")#]<br>
13:[#IsDate("05.06.2026 10:20")#]<br>
14:[#IsDate("13.13.2026 10:20")#]<br>
15:[#IsDate("2026.13.05 10:20")#]<br>
16:[#IsDate("2026.05.32 10:20")#]<br>
17:[#IsDate("2026.13.05")#]<br>
</cfoutput>
