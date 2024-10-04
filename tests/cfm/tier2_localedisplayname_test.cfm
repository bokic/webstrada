<cfscript>
writeOutput("1:[" & GetLocaleDisplayName() & "]");
writeOutput("2:[" & GetLocaleDisplayName("German (Standard)") & "]");
writeOutput("3:[" & GetLocaleDisplayName("fr_FR") & "]");
writeOutput("4:[" & GetLocaleDisplayName("English (US)", "German (Standard)") & "]");
writeOutput("5:[" & GetLocaleDisplayName("de_DE", "fr_FR") & "]");
writeOutput("6:[" & GetLocaleDisplayName("Japanese") & "]");
writeOutput("7:[" & GetLocaleDisplayName("Korean") & "]");
writeOutput("8:[" & GetLocaleDisplayName("Chinese (Hong Kong)") & "]");
writeOutput("9:[" & GetLocaleDisplayName("Norwegian (Nynorsk)") & "]");
writeOutput("10:[" & GetLocaleDisplayName("English (US)") & "]");
writeOutput("11:[" & GetLocaleDisplayName("Swedish") & "]");
writeOutput("12:[" & GetLocaleDisplayName("Chinese (Taiwan)", "Japanese") & "]");
</cfscript>
