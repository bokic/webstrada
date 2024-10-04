<!--- Included by include_in_function_test.cfm. Runs inside the function's
     context: var-declared locals and arguments are shared, non-local
     unqualified writes go to the calling page's variables. --->
<cfoutput>(#localVar#:#arguments.arg1#:#variables.pageX#)</cfoutput>
<cfset localVar = localVar & "_inc">
<cfset Local.newlocal = Local.newlocal & "_inc">
<cfset pageX = pageX & "_inc">
