<cfset x = 5>
<cfset arr = [10, 20]>
<cfset st = {a: 1, key: 7}>
<cfoutput>#x#|#x + 1#|#x * 2#|#x EQ 5#</cfoutput>
<cfoutput>#arr[1]#|#arr[2]#</cfoutput>
<cfoutput>#st.a#|#st.key#|#variables.x#|#variables.st.a#</cfoutput>
<cfoutput>#"v=#x#,#arr[1]#,#st.key#"#</cfoutput>
<cfoutput>#Len("abc")#|#Max(1, 2)#|#(x)#|#x GT 3#</cfoutput>
<cfset inf = 2^1024><cfset nan = 5><cfset infinity = 2>
<cfoutput>#inf#|#nan#|#infinity#|#INF#|#NAN#|#Infinity#</cfoutput>
