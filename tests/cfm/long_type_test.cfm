<cfscript>
// integer literals beyond int32 store as Long and render full digits
writeOutput("L1:" & 2147483648);
writeOutput("|");
writeOutput(9223372036854775807);
writeOutput("|");
writeOutput(9007199254740993);
writeOutput("|");
writeOutput(-2147483649);
writeOutput("<br>");
// DeserializeJSON int64 is exact (previously truncated to int32)
writeOutput("J1:" & DeserializeJSON("2147483648"));
writeOutput("|");
writeOutput(DeserializeJSON("9007199254740993"));
writeOutput("|");
writeOutput(DeserializeJSON("9223372036854775807"));
writeOutput("<br>");
// arithmetic converts Long operands to a computed double like CF
writeOutput("A1:" & (2147483648 + 1));
writeOutput("|");
writeOutput(9223372036854775807 + 0);
writeOutput("|");
writeOutput(9007199254740993 + 1);
writeOutput("|");
writeOutput(2147483647 + 1);
writeOutput("<br>");
// ToString keeps the exact digits
writeOutput("T1:" & ToString(9223372036854775807));
writeOutput("|");
writeOutput(ToString(DeserializeJSON("2147483648")));
writeOutput("<br>");
// Long comparisons stay exact
writeOutput("C1:" & (2147483648 GT 2147483647));
writeOutput("|");
writeOutput(9223372036854775807 EQ 9223372036854775807);
writeOutput("|");
writeOutput(2147483649 EQ 2147483648);
writeOutput("<br>");
// arrays holding Long values render as digits
arr = ArrayNew(1);
ArrayAppend(arr, 2147483648);
ArrayAppend(arr, 9223372036854775807);
writeOutput("R1:" & ArrayToList(arr));
</cfscript>
