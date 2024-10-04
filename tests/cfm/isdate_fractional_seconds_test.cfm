<!--- ParseDateTime with fractional seconds (CF's DateTimeString accepts .ddd
     after :SS) and trailing-garbage rejection around them. --->
<cfoutput>
#TimeFormat(ParseDateTime("10:30:00.123"), "HH:mm:ss")#|
#TimeFormat(ParseDateTime("2020-05-15 10:30:00.5"), "HH:mm:ss")#|
#IsDate("10:30:00.05")#|#IsDate("10:30:00.999")#|#IsDate("10:30:00.999999")#|
#IsDate("10:30:00.999999 trailing")#|#IsDate("10:30:00.")#|#IsDate("10:30:00.bogus")#|
#IsDate("2020-05-15 10:30:00..123")#|#IsDate("2020-05-15 10:30:00.123x")#|
#IsDate("2020-05-15 10:30:00.1.2")#|
#IsDate("2020-05-15 10:30:00.123+05:00bogus")#|#IsDate("2020-05-15 10:30:00+05:00.123")#|
#IsDate("10:30:00+05:00bogus")#|#IsDate("2020-05-15 10:30:00+05:00:30")#
</cfoutput>
