<cfscript>
  // Serialize/DeserializeJSON wrappers (Deserialize itself aborts the CF 2021
  // test server's response, so its JSON behavior is verified via DeserializeJSON)
  ss = Serialize("hello", "json");
  sn = Serialize(42, "json");
  sb = Serialize(true, "json");

  ds = DeserializeJSON('"world"');
  dn = DeserializeJSON('99');
  da = DeserializeJSON('[7,8,9]');
</cfscript><cfoutput>#ss#|#sn#|#sb#|#ds#|#dn#|#da[1]#:#da[2]#:#da[3]#|</cfoutput>
