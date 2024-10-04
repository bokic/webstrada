<cfscript>
// NOTE: this file pins the engine's sqlite-backed Cache* function behavior, but
// it is NOT byte-verified against CF 2025 yet: the caching package is now
// installed on the RDS host, and CF's real behavior diverges (cacheGetProperties
// returns the full ehcache config, cacheSetProperties ignores unknown property
// names, ...). See BUGS_CF.md. The <cfcache> tag IS byte-verified (cfcache_test
// + tier2_cfcache_test).
cacheRemoveAll();
writeOutput("get-missing=[" & cacheGet("missing") & "]");
writeOutput("|put=");
cachePut("id1", "value1");
writeOutput(cacheGet("id1"));
writeOutput("|num=");
cachePut("n1", 42);
writeOutput(cacheGet("n1"));
writeOutput("|exists=" & cacheIdExists("id1"));
writeOutput(":" & cacheIdExists("nope"));
writeOutput("|ids=");
ids = cacheGetAllIds();
writeOutput(arrayLen(ids));
writeOutput(":" & (arrayFind(ids, "id1") GT 0));
writeOutput(":" & (arrayFind(ids, "n1") GT 0));
writeOutput("|struct=");
cachePut("s1", {a:1, b:"x"});
v = cacheGet("s1");
writeOutput(v.a & ":" & v.b);
writeOutput("|array=");
cachePut("arr1", [10,20]);
a = cacheGet("arr1");
writeOutput(a[2]);
writeOutput("|region=");
cachePut("r1", "rv", "", "", "myregion");
writeOutput(cacheGet("r1", "myregion"));
writeOutput(":" & cacheRegionExists("myregion"));
writeOutput("|metadata=");
cachePut("m1", "mv");
cacheGet("m1");
md = cacheGetMetadata("m1");
writeOutput(structKeyExists(md, "NAME"));
writeOutput(":" & md.NAME);
writeOutput(":" & md.HITCOUNT);
writeOutput("|props=");
cacheSetProperties({foo:"bar"});
p = cacheGetProperties("OBJECT");
writeOutput(p.foo);
writeOutput("|remove=");
cachePut("del1", "dv");
cacheRemove("del1");
writeOutput(cacheIdExists("del1"));
writeOutput("|removeall=");
cachePut("k1", 1);
cachePut("k2", 2);
cacheRemoveAll();
writeOutput(arrayLen(cacheGetAllIds()));
writeOutput("|removecachedquery=");
removeCachedQuery("SELECT 1", "test");
writeOutput("ok");
</cfscript>
