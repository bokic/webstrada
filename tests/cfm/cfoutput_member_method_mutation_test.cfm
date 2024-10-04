<cfset st = {x: 10, y: 20}>
<cfset st2 = {a: 1, b: 2, c: 3}>
<cfset st3 = {p: 1, q: 2}>
<!--- Interpreter-path (cfoutput double-hash) member-method mutation write-through.
A mutating member method invoked inside cfoutput must write through to the
caller's variable, matching the JIT path and CF. StructDelete is the mutator CF
tolerates here (it renders YES and the mutation is visible); array mutators
(ArrayPush/Append/Prepend/Clear/DeleteAt/InsertAt/Set/Swap and StructClear/
StructAppend/StructInsert) make ColdFusion abort the request mid-render or
return a divergent value, so the array write-through is covered by the
InterpreterMutatingMethodsWriteThrough unit tests and the JIT path instead. --->
<cfoutput>#st.delete("x")#|#st.keyList()#<br></cfoutput>
<cfoutput>#st.delete("zzz")#|#st.count()#<br></cfoutput>
<cfoutput>#st3.delete("p", true)#|#st3.keyList()#<br></cfoutput>
<cfoutput>#st2.delete("a")#|#st2.delete("b")#|#st2.keyList()#<br></cfoutput>
