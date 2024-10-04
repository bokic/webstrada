<cfscript>
// Reference semantics of argument passing and assignment (verified vs CF 2025).
// Scalars and arrays pass by value; structs and queries by reference.

// scalar arg: caller unchanged
function fScalar(n) { n = 42; }
sn = 1; fScalar(sn);
writeOutput("scalar=#sn# | ");

// array arg: caller unchanged (arrays are value types)
function fArray(ar) { ar[1] = 42; ArrayAppend(ar, 99); }
an = [1,2]; fArray(an);
writeOutput("array=#an[1]#:#arrayLen(an)# | ");

// struct arg: caller mutated (by reference)
function fStruct(s) { s.a = 42; s.newkey = "X"; }
st = {a:1}; fStruct(st);
writeOutput("struct=#st.a#:#st.newkey# | ");

// query arg: caller mutated (by reference)
function fQuery(q) { q.x[1] = "MUT"; }
qn = queryNew("x"); queryAddRow(qn); querySetCell(qn,"x","ORIG",1); fQuery(qn);
writeOutput("query=#qn.x[1]# | ");

// arguments scope mutation propagates for structs
function fArgs() { arguments[1].v = 88; }
sa = {v:1}; fArgs(sa);
writeOutput("args=#sa.v# | ");

// struct assignment aliases (reference type)
s1 = {a:1}; s2 = s1; s2.a = 99;
writeOutput("sassign=#s1.a# | ");

// array assignment copies (value type)
a1 = [1,2]; a2 = a1; a2[1] = 99;
writeOutput("aassign=#a1[1]# | ");

// struct elements inside a copied array stay shared
ao = [{a:1}]; ac = ao; ac[1].a = 55;
writeOutput("arrstruct=#ao[1].a# | ");

// struct returned from a function is shared with the caller's copy
function fRet() { r = {b:5}; return r; }
rv = fRet(); rv.b = 9;
writeOutput("ret=#rv.b# | ");

// pass-and-return keeps the alias
function fPass(s) { return s; }
orig = {c:1}; back = fPass(orig); back.c = 55;
writeOutput("passret=#orig.c# | ");

// nested struct writes propagate through aliases
sx = {inner:{v:1}}; sy = sx; sy.inner.v = 77;
writeOutput("nested=#sx.inner.v# | ");

// StructCopy must return an independent deep copy
sc1 = {k:1}; sc2 = structCopy(sc1); sc2.k = 99;
writeOutput("structcopy=#sc1.k#");
</cfscript>
