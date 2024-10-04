<cfset d = CreateDate(2026, 6, 1) />
<cfoutput>
DayOfWeek: #DayOfWeek(d)#
DayOfYear: #DayOfYear(d)#
DaysInMonth: #DaysInMonth(d)#
DaysInYear: #DaysInYear(d)#
FirstDayOfMonth: #FirstDayOfMonth(d)#
IsDateObject: #IsDateObject(d)#
IsLeapYear: #IsLeapYear(2026)#
IsNumericDate: #IsNumericDate(d)#
Quarter: #Quarter(d)#
Week: #Week(d)#
DayOfWeekAsString: #DayOfWeekAsString(2)#
MonthAsString: #MonthAsString(6)#
ParseDateTime: #DateFormat(ParseDateTime("2026-06-01"), "yyyy-mm-dd")#
DatePart: #DatePart("yyyy", d)#
DateAdd: #DateFormat(DateAdd("d", 10, d), "yyyy-mm-dd")#
DateDiff: #DateDiff("d", d, DateAdd("d", 10, d))#
DateCompare: #DateCompare(d, DateAdd("d", 10, d))#
CreateTimeSpan: #CreateTimeSpan(1, 2, 3, 4)#
GetHttpTimeString: #GetHttpTimeString(d)#
</cfoutput>
