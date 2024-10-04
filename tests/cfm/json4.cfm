<cfscript>
  // Serialize wrapper with explicit type argument
  ss = Serialize("hello", "json");
  sn = Serialize(42, "json");
  sb = Serialize(true, "json");
  a = ArrayNew(1);
  ArrayAppend(a, 1);
  ArrayAppend(a, "x");
  sa = Serialize(a, "json");
  st = StructNew();
  StructInsert(st, "k", "v");
  so = Serialize(st, "json");
</cfscript><cfoutput>#ss#|#sn#|#sb#|#sa#|#so#|</cfoutput>
