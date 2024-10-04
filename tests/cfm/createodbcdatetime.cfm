<cfset d = CreateDateTime(2024, 5, 15, 13, 45, 30)>
<cfset odbcDt = CreateODBCDateTime(d)>
<cfset d2 = CreateDateTime(2024, 5, 15, 13, 45, 30)>

<cfoutput>
DirectDateTime:#odbcDt#|
FromDateTime:#CreateODBCDateTime(CreateDateTime(2024, 5, 15, 13, 45, 30))#|
FromStringWithTime:#CreateODBCDateTime("2024-05-15 13:45:30")#|
FromStringDateOnly:#CreateODBCDateTime("2024-05-15")#|
FromSerial:#CreateODBCDateTime(2004)#|
FromNow:#IsDate(CreateODBCDateTime(Now()))#|
DateTimeFormat:#DateTimeFormat(odbcDt, "yyyy-mm-dd HH:nn:ss")#|
DateFormat:#DateFormat(CreateODBCDateTime("2024-05-15"), "yyyy-mm-dd")#|
TimeFormat:#TimeFormat(CreateODBCDateTime("2024-05-15 13:45:30"), "HH:mm:ss")#|
DateDiffSec:#DateDiff("s", d, odbcDt)#|
DateDiffDay:#DateDiff("d", d, CreateODBCDateTime("2024-05-15 13:45:30"))#|
Year:#Year(odbcDt)#|
Month:#Month(odbcDt)#|
Day:#Day(odbcDt)#|
Hour:#Hour(odbcDt)#|
Minute:#Minute(odbcDt)#|
Second:#Second(odbcDt)#|
IsDate:#IsDate(odbcDt)#|
IsNumericDate:#IsNumericDate(odbcDt)#|
EqSameDate:#(odbcDt EQ d2)#|
EqPlainDate:#(CreateODBCDateTime(d) EQ d)#|
EqDateString:#(odbcDt EQ "2024-05-15 13:45:30")#|
NeqDifferent:#(odbcDt NEQ CreateDateTime(2024, 5, 15, 0, 0, 0))#|
GtEarlier:#(odbcDt GT CreateDateTime(2024, 5, 15, 0, 0, 0))#|
LtLater:#(odbcDt LT CreateDateTime(2024, 5, 15, 23, 0, 0))#|
DateCompare:#DateCompare(odbcDt, d2)#|
</cfoutput>

<cftry>
    <cfoutput>#CreateODBCDateTime("bad-date")#|</cfoutput>
<cfcatch type="any">
    <cfoutput>ErrType:#cfcatch.type#|ErrMsg:#cfcatch.message#</cfoutput>
</cfcatch>
</cftry>

<cfscript>
script_dt = CreateODBCDateTime(CreateDateTime(2025, 12, 31, 23, 59, 59));
script_str = CreateODBCDateTime("2025-12-31 23:59:59");
script_eq = (script_dt EQ script_str);
</cfscript>
<cfoutput>
ScriptFromDateTime:#script_dt#|
ScriptFromString:#script_str#|
ScriptEq:#script_eq#|
</cfoutput>
