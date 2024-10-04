<cfoutput>#Left(ToString(pi), 32)#</cfoutput>
<cfoutput>#IsObject(pi)#|#IsSimpleValue(pi)#|#IsCustomFunction(pi)#|#IsClosure(pi)#|#IsStruct(pi)#|#IsArray(pi)#|#IsNumeric(pi)#|#IsBoolean(pi)#</cfoutput>
<cfset pi = 99><cfoutput>#pi#</cfoutput>
<cfset fn = "abs"><cfoutput>#IsObject(pi)#</cfoutput>
<cfoutput>#Left(ToString(abs), 32)#</cfoutput>
<cfoutput>#Left(ToString(len), 32)#|#Left(ToString(ucase), 32)#</cfoutput>
<cfoutput>"x #Left(ToString(bitand), 32)# y"</cfoutput>
