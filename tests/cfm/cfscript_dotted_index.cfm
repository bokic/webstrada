<cfscript>
s = {k:["a","b","c"]};
t = {a:{b:[10,20,30]}};
arr = [[1,2],[3,4]];
writeOutput(s.k[2] & "|" & s.k[3] & "|");
writeOutput(t.a.b[2] & "|");
writeOutput(arr[2][1] & "|");
</cfscript>
