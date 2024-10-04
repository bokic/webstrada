<cfset d = CreateDateTime(2020, 5, 15, 14, 30, 45)>
<cfoutput>DSH[#DateFormat(d, "full")#]</cfoutput>

<cfset r = SetLocale("French (Standard)")>
<cfoutput>FR[#GetLocale()#][#LSDateFormat(d, "short")#][#LSDateFormat(d, "medium")#][#LSDateFormat(d, "long")#][#LSDateFormat(d, "full")#][#LSTimeFormat(d, "short")#][#LSTimeFormat(d, "medium")#][#LSDateFormat(d, "mmmm")#][#LSDateFormat(d, "ddd")#][#LSNumberFormat(1234.5)#][#LSCurrencyFormat(1234.5)#][#LSCurrencyFormat(1234.5, "international")#][#LSCurrencyFormat(1234.5, "none")#][#LSCurrencyFormat(-1234.5)#]</cfoutput>

<cfset r = SetLocale("German (Standard)")>
<cfoutput>DE[#r#][#GetLocale()#][#LSDateFormat(d, "short")#][#LSDateFormat(d, "medium")#][#LSTimeFormat(d, "short")#][#LSNumberFormat(1234.56, "9,999.99")#][#LSNumberFormat(1234.56, "_")#][#LSCurrencyFormat(1234.5)#][#LSCurrencyFormat(-1234.5)#][#LSParseNumber("1.234,5")#]</cfoutput>

<cfset r = SetLocale("English (US)")>
<cfoutput>EN[#r#][#GetLocale()#][#LSDateFormat(d, "short")#][#LSDateFormat(d, "medium")#][#LSDateFormat(d, "full")#][#LSTimeFormat(d, "short")#][#LSTimeFormat(d, "medium")#][#LSNumberFormat(1234.5)#][#LSNumberFormat(1234.56, "9,999.99")#][#LSNumberFormat(0.125, "0.00")#][#LSNumberFormat(5, "9999")#][#LSNumberFormat(-5, "9999")#][#LSCurrencyFormat(1234.5)#][#LSCurrencyFormat(1234.5, "international")#][#LSCurrencyFormat(-1234.5)#][#LSParseNumber("1,234.5")#][#LSParseCurrency("$1,234.50")#][#LSIsCurrency("$1,234.50")#]</cfoutput>

<cfset r = SetLocale("English (UK)")>
<cfoutput>UK[#GetLocale()#][#LSDateFormat(d, "short")#][#LSDateFormat(d, "medium")#][#LSTimeFormat(d, "short")#][#LSCurrencyFormat(1234.5)#]</cfoutput>

<cfset r = SetLocale("Japanese")>
<cfoutput>JA[#GetLocale()#][#LSDateFormat(d, "short")#][#LSDateFormat(d, "medium")#][#LSDateFormat(d, "full")#][#LSNumberFormat(1234.5)#][#LSCurrencyFormat(1234.5)#][#LSDateFormat(d, "mmmm")#]</cfoutput>

<cfset r = SetLocale("Korean")>
<cfoutput>KO[#GetLocale()#][#LSTimeFormat(d, "short")#][#LSDateFormat(d, "full")#]</cfoutput>

<cfset r = SetLocale("Chinese (China)")>
<cfoutput>ZH[#GetLocale()#][#LSTimeFormat(d, "short")#][#LSDateFormat(d, "full")#]</cfoutput>

<cfset r = SetLocale("it_IT")>
<cfoutput>IT[#GetLocale()#][#LSDateFormat(d, "short")#][#LSDateFormat(d, "medium")#][#LSDateFormat(d, "long")#][#LSTimeFormat(d, "short")#][#LSCurrencyFormat(1234.5)#]</cfoutput>

<cfset r = SetLocale("Spanish (Standard)")>
<cfoutput>ES[#LSDateFormat(d, "short")#][#LSDateFormat(d, "medium")#][#LSDateFormat(d, "long")#][#LSCurrencyFormat(1234.5)#][#LSCurrencyFormat(1234.5, "international")#]</cfoutput>

<cfset r = SetLocale("Portuguese (Standard)")>
<cfoutput>PT[#LSDateFormat(d, "long")#][#LSDateFormat(d, "full")#][#LSCurrencyFormat(1234.5)#]</cfoutput>

<cfset r = SetLocale("Swedish")>
<cfoutput>SV[#LSDateFormat(d, "short")#][#LSDateFormat(d, "medium")#][#LSDateFormat(d, "long")#]</cfoutput>

<cfset r = SetLocale("French (Canadian)")>
<cfoutput>FRCA[#LSDateFormat(d, "short")#][#LSDateFormat(d, "medium")#][#LSCurrencyFormat(-1234.5)#]</cfoutput>
