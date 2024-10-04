<cfoutput>
#("1" eq 1)#|#("1.5" eq 1.5)#|#("abc" eq 1)#|#(1 eq "1.0")#|#("1" neq 2)#|#(" 1 " eq 1)#|#("1e2" eq 100)#|#("" eq 0)#|#("0" eq 0)#|#("true" eq true)#
#("1" eq true)#|#("yes" eq true)#|#("no" eq false)#|#("abc" eq false)#|#(1 eq true)#|#("0" eq false)#|#("" eq "")#|#("A" eq "a")#
#("1.5" eq true)#|#(2 eq true)#|#("2" eq true)#|#("0.0" eq false)#|#("10" eq 5)#|#("abc" eq "abc")#|#(1.0 eq "1")#|#("" eq false)#|#("yes" eq "true")#
#(CreateDateTime(2024,5,15,13,45,30) eq 45427)#|#(CreateDateTime(2024,5,15,13,45,30) eq "2024-05-15 13:45:30")#|#(5 eq "hello")#|#("bar" eq 1)#|#("apple" eq "apple")#
#("10" GT 5)#|#("abc" GT 5)#|#("b" GT "a")#|#("5" GT "10")#|#(CreateDateTime(2024,5,15,13,45,30) GT 45427)#|#("1" LT 2)#
</cfoutput>
