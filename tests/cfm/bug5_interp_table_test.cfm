<cfscript>
x = 5;
writeOutput("basic=#x#|");
writeOutput("escaped=##|");
writeOutput("interp-char-escaped=#x#a##|");
writeOutput("interp-space-escaped=#x# ##|");
writeOutput("escaped-only=a##b|");
</cfscript>
