<cfset arr = ArrayNew(1)>
<cfset ArrayAppend(arr, 1)>
<cfset ArrayAppend(arr, "two")>
<cfset st = StructNew()>
<cfset StructInsert(st, "X", 1)>
<cfset dt = ParseDateTime("2026-08-01 13:59:51")>
<cfset a = 1/8>
<cfset b = 10/4>
<cfset c = 1/10>
<cfset d = 1/100>
<cfset e = 1/1000>
<cfset f = 1/10000>
<cfset g = 2.0*3>
<cfset h = Chr(9) & "42">
<cfset i = "a" & Chr(10) & "b">
<cfoutput>#ToString("hello")#|#ToString(42)#|#ToString(-12)#|#ToString(true)#|#ToString(false)#|#Len(ToString(""))#|#ToString(3.14)#|#ToString(0.5)#|#ToString(a)#|#ToString(b)#|#ToString(c)#|#ToString(d)#|#ToString(e)#|#ToString(f)#|#ToString(12345.6789)#|#ToString(g)#|#ToString(100000)#|#ToBase64(CharsetDecode(ToString(ToBinary("w6k=")), "UTF-8"))#|#ToBase64(CharsetDecode(ToString(ToBinary("w6k="), "UTF-8"), "UTF-8"))#|#Len(ToString(ToBinary("")))#|#ToString(ParseDateTime("2026-08-01 13:59:51"))#|#Val("123abc")#|#Val("-12.5x")#|#Val("1e3")#|#Val("12.5.6")#|#Val("0x10")#|#Val("1,234")#|#Val("--5")#|#Val("5-3")#|#Val("+5")#|#Val("3.14e2xyz")#|#Val("  42")#|#Val("  12.5  ")#|#Val("-")#|#Val("")#|#Val("abc123")#|#Val("1.0")#|#Val("10.00")#|#Val(".5")#|#Val("-12")#|#Val("-0.25")#|#Val(3.14)#|#Val(true)#|#Val(123)#|#Val("1.")#|#Val(h)#|#Val("-0.0")#|#Val("00.50")#|#Val("  .25")#|#ToScript("hello", "x")#|#ToScript(42, "y")#|#ToScript(3.14, "z")#|#ToScript(true, "b")#|#ToScript("it's", "q")#|#ToScript(Chr(92), "bs")#|#ToScript(Chr(13), "cr")#|#ToScript(Chr(9), "tb")#|#ToScript(i, "lf")#|#ToScript("x", "v", false)#|#ToScript(arr, "arr")#|#ToScript(st, "st")#|#ToScript(dt, "dt")#</cfoutput>
