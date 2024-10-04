<cfscript>
  // SerializeJSON: string, number, boolean, array, struct, nested
  s = SerializeJSON("hello");
  n = SerializeJSON(42);
  bt = SerializeJSON(true);
  bf = SerializeJSON(false);
  fl = SerializeJSON(3.14);
  
  a = ArrayNew(1);
  ArrayAppend(a, 1);
  ArrayAppend(a, "two");
  ArrayAppend(a, true);
  sa = SerializeJSON(a);
  
  st = StructNew();
  StructInsert(st, "name", "test");
  StructInsert(st, "value", 42);
  ss = SerializeJSON(st);
  
  a2 = ArrayNew(1);
  st2 = StructNew();
  StructInsert(st2, "x", 10);
  ArrayAppend(a2, st2);
  sn = SerializeJSON(a2);
</cfscript><cfoutput>#s#|#n#|#bt#|#bf#|#fl#|#sa#|#ss#|#sn#|</cfoutput>