<cfscript>
  // DeserializeJSON: string, number, boolean, array, struct, nested
  ds = DeserializeJSON('"hello"');
  dn = DeserializeJSON('42');
  db = DeserializeJSON('true');
  da = DeserializeJSON('[1,2,3]');
  dst = DeserializeJSON('{"name":"test","count":42}');
  
  dnested = DeserializeJSON('{"items":[{"id":1},{"id":2}]}');
  item1 = dnested.ITEMS[1];
  item2 = dnested.ITEMS[2];
  
  // strictMapping=false
  da2 = DeserializeJSON('[4,5,6]', false);
</cfscript><cfoutput>#ds#|#dn#|#db#|#da[1]#:#da[2]#:#da[3]#|#dst.NAME#:#dst.COUNT#|#item1.ID#:#item2.ID#|#da2[1]#:#da2[2]#:#da2[3]#|</cfoutput>