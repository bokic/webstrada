<cfscript>
nl = Chr(10);
fileWrite(ExpandPath("t2_props.properties"), "a=1" & nl & "b = 2" & nl & "c: 3" & nl & "d=hello world" & nl);
writeOutput("1:[" & GetPropertyString(ExpandPath("t2_props.properties"), "a") & "]");
writeOutput("2:[" & GetPropertyString(ExpandPath("t2_props.properties"), "b") & "]");
writeOutput("3:[" & GetPropertyString(ExpandPath("t2_props.properties"), "c") & "]");
writeOutput("4:[" & GetPropertyString(ExpandPath("t2_props.properties"), "d") & "]");
writeOutput("5:[" & GetPropertyString(ExpandPath("t2_props.properties"), "nope") & "]");
p = GetPropertyFile(ExpandPath("t2_props.properties"));
writeOutput("6:[" & IsStruct(p) & "]");
writeOutput("7:[" & p.a & "]");
writeOutput("8:[" & p.b & "]");
writeOutput("9:[" & p.c & "]");
</cfscript>
