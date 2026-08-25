<cfscript>
base = CreateDateTime(2026, 8, 25, 14, 30, 45);

r = base.setYear(2016);
WriteOutput("setYear return=" & Year(r) & "-" & Month(r) & "-" & Day(r));
WriteOutput(" base=" & Year(base) & "-" & Month(base) & "-" & Day(base));
WriteOutput("|");

r = base.setMonth(3);
WriteOutput("setMonth return=" & Year(r) & "-" & Month(r) & "-" & Day(r));
WriteOutput(" base=" & Year(base) & "-" & Month(base) & "-" & Day(base));
WriteOutput("|");

r = base.setDay(15);
WriteOutput("setDay return=" & Year(r) & "-" & Month(r) & "-" & Day(r));
WriteOutput(" base=" & Year(base) & "-" & Month(base) & "-" & Day(base));
WriteOutput("|");

r = base.setHour(10);
WriteOutput("setHour return=" & Hour(r) & ":" & Minute(r) & ":" & Second(r));
WriteOutput(" base=" & Hour(base) & ":" & Minute(base) & ":" & Second(base));
WriteOutput("|");

r = base.setMinute(55);
WriteOutput("setMinute return=" & Hour(r) & ":" & Minute(r) & ":" & Second(r));
WriteOutput(" base=" & Hour(base) & ":" & Minute(base) & ":" & Second(base));
WriteOutput("|");

r = base.setSecond(9);
WriteOutput("setSecond return=" & Hour(r) & ":" & Minute(r) & ":" & Second(r));
WriteOutput(" base=" & Hour(base) & ":" & Minute(base) & ":" & Second(base));
</cfscript>
