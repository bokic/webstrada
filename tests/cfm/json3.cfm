<cfscript>
  t1 = IsJSON('{"a":1}');
  t2 = IsJSON('[1,2,3]');
  t3 = IsJSON('"string"');
  t4 = IsJSON('42');
  t5 = IsJSON('true');
  f1 = IsJSON("not json");
  f2 = IsJSON("");
  f3 = IsJSON("{invalid}");
</cfscript><cfoutput>#t1#|#t2#|#t3#|#t4#|#t5#|#f1#|#f2#|#f3#|</cfoutput>