<cfscript>
    values = [1];
    arrayAppend(values, [2, 3], true);
    writeOutput(arrayLen(values) & ":" & values[2] & ":" & values[3] & "|");

    nested = [1];
    arrayAppend(nested, [2, 3]);
    writeOutput(arrayLen(nested) & ":" & arrayLen(nested[2]));
</cfscript>
