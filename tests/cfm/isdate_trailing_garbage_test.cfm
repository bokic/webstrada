<!--- IsDate/ParseDateTime full-consumption: a trailing non-whitespace tail must
     be rejected (was BUGS.md "IsDate()/ParseDateTime() ignore trailing
     garbage"). Fractional seconds and a trailing timezone offset are accepted
     per CF's CFDateTimeParser (verified on CF 2025). --->
<cfoutput>
#IsDate("2020-05-15bogus")#|#IsDate("2020-05-15 10:30:00bogus")#|#IsDate("2020-05-15Tbogus")#|
#IsDate("2020-05-15 10:30:00.123")#|#IsDate("10:30:00.123")#|#IsDate("2020-05-15 10:30:00 ")#|
#IsDate("2020-05-15 10:30:00.123 trailing")#|#IsDate("2020-05-15 10:30:00Zbogus")#|
#IsDate("2020-05-15 10:30:00+05:00")#|#IsDate("2020-05-15 10:30:00+05:00bogus")#|#IsDate("2020-05-15 10:30:00Z")#|
#IsDate("10:30:00.5")#|#IsDate("2020-05-15 10:30.5")#|#IsDate("2020-05-15.123")#|
#IsDate("2020-05-15 10:30:00,123")#|#IsDate("2020-05-15 10:30:00.123+05:00")#|
#IsDate("2020-05-15 10:30:00+05")#|#IsDate("2020-05-15 10:30:00+0530")#|#IsDate("2020-05-15 10:30:00+0530bogus")#|
#IsDate("2020-05-15+05:00")#|#IsDate("10:30:00+05:00")#|#IsDate("10:30:00Z")#|
#IsDate("2020-05-15 10:30")#|#IsDate("2020-05-15 10:30bogus")#|
#IsDate("2020-05-15T10:30:00.123")#|#IsDate("2020-05-15 10:30:00.123Z")#|
#IsDate("{ts '2020-05-15 10:30:00'}")#|#IsDate("{ts '2020-05-15 10:30:00'}bogus")#
</cfoutput>
