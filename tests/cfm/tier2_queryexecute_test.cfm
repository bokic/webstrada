<cfscript>
// QueryExecute tests: positional and named params, list params, options.
queryExecute("DROP TABLE IF EXISTS qe_users", {}, {datasource:"webstrada"});
queryExecute("CREATE TABLE qe_users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, age INTEGER)", {}, {datasource:"webstrada"});
queryExecute("INSERT INTO qe_users (name, age) VALUES ('alice', 30)", {}, {datasource:"webstrada"});
queryExecute("INSERT INTO qe_users (name, age) VALUES ('bob', 25)", {}, {datasource:"webstrada"});
queryExecute("INSERT INTO qe_users (name, age) VALUES ('carol', 35)", {}, {datasource:"webstrada"});

// positional
q = queryExecute("SELECT id, name, age FROM qe_users WHERE age > ? ORDER BY id", [20], {datasource:"webstrada"});
writeOutput("A:[" & q.recordcount & "]");
writeOutput("B:[" & q.name[1] & "]");
writeOutput("C:[" & q.age[3] & "]");

// named
qn = queryExecute("SELECT name FROM qe_users WHERE age = :targetAge", {targetAge:25}, {datasource:"webstrada"});
writeOutput("D:[" & qn.name[1] & "]");

// named, no params provided but unused
qempty = queryExecute("SELECT COUNT(*) AS c FROM qe_users", {}, {datasource:"webstrada"});
writeOutput("E:[" & qempty.c & "]");

// list param
ql = queryExecute("SELECT name FROM qe_users WHERE age IN (:ages)", {ages:{value:"25,30", list:true}}, {datasource:"webstrada"});
writeOutput("F:[" & ql.recordcount & "]");

// list with custom separator
ql2 = queryExecute("SELECT name FROM qe_users WHERE age IN (:ages)", {ages:{value:"30|35", list:true, separator:"|"}}, {datasource:"webstrada"});
writeOutput("G:[" & ql2.recordcount & "]");

// string param with quote
qstr = queryExecute("SELECT name FROM qe_users WHERE name = ?", ["O'Brien"], {datasource:"webstrada"});
writeOutput("H:[" & qstr.recordcount & "]");

// integer param as struct value
qi = queryExecute("SELECT name FROM qe_users WHERE age = :a", {a:{value:35, cfsqltype:"CF_SQL_INTEGER"}}, {datasource:"webstrada"});
writeOutput("I:[" & qi.name[1] & "]");

// result option
r = queryExecute("SELECT name FROM qe_users WHERE age = 30", {}, {datasource:"webstrada", result:"resMeta"});
writeOutput("J:[" & resMeta.recordcount & "]");
writeOutput("K:[" & resMeta.sql & "]");

// no params at all
qnp = queryExecute("SELECT name FROM qe_users ORDER BY id", {}, {datasource:"webstrada"});
writeOutput("L:[" & qnp.recordcount & "]");
</cfscript>
