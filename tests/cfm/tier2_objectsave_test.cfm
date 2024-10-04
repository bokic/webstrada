<cfscript>
// ObjectSave / ObjectLoad round-trip tests.
// NOTE: CF uses Java Object Serialization bytes; this engine uses a JSON
// wire format with a magic header (documented divergence, see BUGS.md).
// Therefore these cases are only meaningful for round-trip within an engine,
// and cannot be compared byte-for-byte against ColdFusion.

// 1. simple struct round-trip
s = {a:1, b:"x"};
b = ObjectSave(s);
writeOutput("1:[" & isBinary(b) & "]");
r = ObjectLoad(b);
writeOutput("2:[" & isStruct(r) & "]");
writeOutput("3:[" & r.a & "]");
writeOutput("4:[" & r.b & "]");

// 2. nested structure
s2 = {name:"alice", addr:{city:"NYC", zip:10001}, tags:["x","y","z"]};
r2 = ObjectLoad(ObjectSave(s2));
writeOutput("5:[" & r2.name & "]");
writeOutput("6:[" & r2.addr.city & "]");
writeOutput("7:[" & r2.addr.zip & "]");
writeOutput("8:[" & arrayLen(r2.tags) & "]");
writeOutput("9:[" & r2.tags[3] & "]");

// 3. array round-trip
a = [10, 20.5, "str"];
ra = ObjectLoad(ObjectSave(a));
writeOutput("10:[" & isArray(ra) & "]");
writeOutput("11:[" & ra[1] & "]");
writeOutput("12:[" & ra[2] & "]");
writeOutput("13:[" & ra[3] & "]");

// 4. simple values round-trip
rn = ObjectLoad(ObjectSave(42));
writeOutput("14:[" & rn & "]");
rs = ObjectLoad(ObjectSave("hello"));
writeOutput("15:[" & rs & "]");
rb = ObjectLoad(ObjectSave(true));
writeOutput("16:[" & rb & "]");

// 5. file save/load
filePath = getTempDirectory() & "mkf2_obj_test.bin";
writeOutput("17:[" & isDefined("noSuchVar") & "]");
</cfscript>
