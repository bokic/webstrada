<!--- PrecisionEvaluate: BigDecimal-arithmetic expression evaluation, verified
     byte-for-byte against CF 2025 (the RDS host). + - * are exact
     BigDecimal; / uses CF's _divideBD (exact at the dividend scale, else scale
     20 half-even); ^ is double math converted to BigDecimal's exact decimal
     expansion; MOD/% and \ are integer ops; results render with
     BigDecimal.toString(). --->
<cfoutput>[#PrecisionEvaluate("1/3")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("2/3")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1+2*3")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("10/4")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("100/8")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("0.1+0.2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1.5*2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("2^10")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("10 MOD 3")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("10\3")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1/7")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("5/2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("-5/2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("2.50+1")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("123456789.123456789 * 100")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("123456789123456789123456789 + 1")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("0.0000001 * 0.0000001")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("10000000000000000000000 * 1.5")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1/10000000")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("0.5^10")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("2^100")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("-2^2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("2^0.5")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("(1/3)")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1 + 2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("7.5 % 2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1.5e3")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("5 - -3")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("0.1+0.2 == 0.3")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("5.00/2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("0.1/0.2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1.000/8")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("123456.789/1000")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1/6")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("22/7")#]</cfoutput>
<cfset pe_x = 3>
<cfset pe_pi = "3.14159">
<cfoutput>[#PrecisionEvaluate("pe_x * 2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("pe_pi * 2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("pe_x")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1+1, 2+2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("true")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("false")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("2^0")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1 EQ 2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1 LT 2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1 GTE 2")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("1 IS 1")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("NOT 0 GT 3")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("(1 + 2) * 3")#]</cfoutput>
<cfoutput>[#PrecisionEvaluate("'a' & 'b'")#]</cfoutput>
