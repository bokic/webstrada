<cfset v_acos = Round(ACos(0.5) * 10000) / 10000>
<cfset v_asin = Round(ASin(0.5) * 10000) / 10000>
<cfset v_atn = Round(Atn(0.5) * 10000) / 10000>
<cfset v_cos = Round(Cos(1.0) * 10000) / 10000>
<cfset v_exp = Round(Exp(1.0) * 10000) / 10000>
<cfset v_log = Round(Log(10.0) * 10000) / 10000>
<cfset v_pi = Round(Pi() * 10000) / 10000>
<cfset v_sin = Round(Sin(1.0) * 10000) / 10000>
<cfset v_tan = Round(Tan(1.0) * 10000) / 10000>
<cfoutput>
ACos:#v_acos#
ASin:#v_asin#
Atn:#v_atn#
Ceiling:#Ceiling(3.14)#
Cos:#v_cos#
Exp:#v_exp#
Floor:#Floor(3.8)#
Int:#Int(3.99)#
Log:#v_log#
Log10:#Log10(10.0)#
Max:#Max(15, 30)#
Min:#Min(15, 30)#
Pi:#v_pi#
Round:#Round(3.5)#
Sgn:#Sgn(-15)#
Sin:#v_sin#
Sqr:#Sqr(25)#
Tan:#v_tan#
</cfoutput>

