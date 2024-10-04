<cfset num1 = 123456.789 />
<cfset num2 = 123456 />
<cfset num3 = -1234.5 />
<cfset num4 = 0.1 />
<cfset p1 = "Hello" />
<cfset p2 = "Hello
World" />
<cfset p3 = "Hello

World" />
<cfset p4 = "Hello

" />
<cfset p5 = "Hello
" />
<cfset p6 = "
Hello" />
<cfset p7 = "

Hello" />
<cfoutput>#DecimalFormat(num1)#|#DecimalFormat(num2)#|#DecimalFormat(num3)#|#DecimalFormat(num4)#|#DollarFormat(num1)#|#DollarFormat(num2)#|#DollarFormat(num3)#|#DollarFormat(num4)#|#YesNoFormat(true)#|#YesNoFormat(false)#|#YesNoFormat(42)#|#YesNoFormat(0)#|#YesNoFormat("yes")#|#YesNoFormat("no")#|#YesNoFormat("1")#|#YesNoFormat("0")#|[#ParagraphFormat(p1)#]|[#ParagraphFormat(p2)#]|[#ParagraphFormat(p3)#]|[#ParagraphFormat(p4)#]|[#ParagraphFormat(p5)#]|[#ParagraphFormat(p6)#]|[#ParagraphFormat(p7)#]</cfoutput>
