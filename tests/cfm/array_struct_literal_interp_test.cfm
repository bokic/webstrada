<cfoutput>
I1=#ArrayLen([1,2,3])#|I2=#(ArrayLen([1,2,3]))#|I3=#ArrayLen([[1],[2,3]])#|I4=#[1,2][2]#|I5=#{a:5}.a#|I6=#{a:5}["a"]#|I7=#ArrayLen([1, "x", true])#|I8=#{x:{y:7}}.x.y#|I10=#ArrayIsEmpty([])#|I11=#StructCount({a:1,b:2})#|I12=#ArrayLen([1+1, 2*3])#
</cfoutput>
