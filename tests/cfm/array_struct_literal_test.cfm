<cfset arr = [10, 20, 30]><cfoutput>#arr[1]#|#arr[2]#|#arr[3]#|#ArrayLen(arr)#</cfoutput>
<cfset s = {a:1,b:2}><cfoutput>#s.a#|#s.b#|#s["a"]#</cfoutput>
<cfset e = []><cfoutput>[#ArrayLen(e)#]</cfoutput>
<cfset n = [[1,2],[3,4]]><cfoutput>#n[1][1]#|#n[2][2]#|#ArrayLen(n)#|#ArrayLen(n[1])#</cfoutput>
<cfset q = {"k":5,"j":6}><cfoutput>#q.k#|#q["j"]#</cfoutput>
<cfset m = [1, "two", true, [7,8]]><cfoutput>#m[1]#|#m[2]#|#m[3]#|#m[4][2]#</cfoutput>
<cfset o = {x: {y: 9}}><cfoutput>#o.x.y#</cfoutput>
<cfset arr2 = [1,2][1]><cfoutput>#arr2#</cfoutput>
