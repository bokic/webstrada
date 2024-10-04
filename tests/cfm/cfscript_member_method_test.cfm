<cfscript>
// Member-method calls: `value.method(args)` dispatches the built-in function
// with the receiver as the first argument. String/array/struct/query/date
// methods plus a stored UDF shadowing the built-in table.
writeOutput("hello".toUpperCase() & "|");
writeOutput("HELLO".toLowerCase() & "|");
writeOutput("hello".mid(2, 3) & "|");
writeOutput("  x  ".trim() & "|");
writeOutput("hello".len() & "|");
writeOutput("hello".reverse() & "|");
writeOutput("hello".replace("l", "L", "all") & "|");
writeOutput("hello world".find("world") & "|");
writeOutput("hello".left(2) & "|");
writeOutput("hello".right(2) & "|");
writeOutput("a,b,c".listLen() & "|");
writeOutput("a,b,c".listGetAt(2) & "|");
writeOutput("a,b,c".listAppend("d") & "|");
writeOutput("a,b,c".listToArray()[1] & "|");
writeOutput("<br>");

arr = [3, 1, 2];
writeOutput(arr.len() & "|");
writeOutput(arr.max() & "|");
writeOutput(arr.min() & "|");
writeOutput(arr.sum() & "|");
writeOutput(arr.toList() & "|");
writeOutput(arr.contains(2) & "|");
writeOutput("<br>");

arr.append(4);
writeOutput("append:" & arr[4] & ":" & arr.len() & "|");
arr.prepend(0);
writeOutput("prepend:" & arr[1] & ":" & arr.len() & "|");
arr.push(5);
writeOutput("push:" & arr[6] & "|");
arr.delete(2);
writeOutput("delete:" & arr[2] & ":" & arr.len() & "|");
arr.clear();
writeOutput("clear:" & arr.len() & "|");
arr.resize(3);
writeOutput("resize:" & arr.len() & "|");
writeOutput("<br>");

st = {x: 10, y: 20};
writeOutput(st.keyExists("x") & "|");
writeOutput(st.count() & "|");
writeOutput(st.isEmpty() & "|");
writeOutput(st.keyList() & "|");
writeOutput(st.find("y") & "|");
writeOutput(st.keyArray()[1] & "|");
writeOutput(st.valueArray()[2] & "|");
st.delete("x");
writeOutput("del:" & st.keyExists("x") & ":" & st.count() & "|");
st.clear();
writeOutput("cleared:" & st.count() & "|");
writeOutput("<br>");

q = queryNew("a,b");
q.addRow();
querySetCell(q, "a", 1, 1);
querySetCell(q, "b", 2, 1);
writeOutput(q.recordCount & "|");
writeOutput(q.columnList & "|");
writeOutput(q.len() & "|");
writeOutput("<br>");

dt = createDateTime(2024, 5, 6, 7, 8, 9);
writeOutput(dt.year() & "|");
writeOutput(dt.month() & "|");
writeOutput(dt.day() & "|");
writeOutput(dt.hour() & "|");
writeOutput(dt.minute() & "|");
writeOutput(dt.second() & "|");
writeOutput(dt.dateFormat("yyyy-mm-dd") & "|");
writeOutput(dt.datePart("m") & "|");
writeOutput(dt.add("d", 2).dateFormat("yyyy-mm-dd") & "|");
writeOutput("<br>");

s = {k: ["hello", "world"]};
writeOutput(s.k[2].toUpperCase() & "|");
writeOutput(s.k[2].len() & "|");
writeOutput(s.k[2].mid(1, 3) & "|");
writeOutput("<br>");

// A UDF stored in a struct member is invoked; it shadows built-ins.
st2 = {};
st2.say = function(v) { return "Hi " & arguments.v; };
writeOutput(st2.say("x") & "|");
st3 = {};
st3.len = function() { return 99; };
writeOutput(st3.len() & "|");
</cfscript>
