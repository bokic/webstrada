<cfscript>
// ajaxLink / ajaxOnLoad / invokeCFClientFunction tests.
r = ajaxLink("http://example.com/page");
writeOutput("A:[" & r & "]");
ajaxOnLoad("myFunc");
writeOutput("B:END;");
</cfscript>
