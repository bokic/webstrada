<cfset dt = CreateDateTime(2026, 5, 31, 14, 30, 45) />
<cfset d = CreateDate(2026, 5, 31) />
<cfset t = CreateTime(14, 30, 45) />
<cfoutput>#DateFormat(d)#|#DateFormat(d, "yyyy-mm-dd")#|#DateFormat(d, "dd/mm/yyyy")#|#DateFormat(d, "d-m-yy")#|#DateFormat(d, "mmmm d, yyyy")#|#DateFormat(d, "mmm d, yyyy")#|#TimeFormat(t)#|#TimeFormat(t, "hh:mm:ss tt")#|#TimeFormat(t, "h:m:s t")#|#TimeFormat(t, "HH:mm:ss")#|#DateTimeFormat(dt)#|#DateTimeFormat(dt, "yyyy-mm-dd HH:mm:ss")#|#IsDate(dt)#|#IsDate("2026-05-31 14:30:45")#|#IsDate("05/31/2026")#|#IsDate("hello")#|#Year(dt)#|#Month(dt)#|#Day(dt)#|#Hour(dt)#|#Minute(dt)#|#Second(dt)#</cfoutput>
