<cfimport prefix="mytag" taglib="customtags">
<cfset callervar = "initial_val">
<mytag:simple name="Alice" />
<cfoutput>[AFTER:#callervar#]</cfoutput>
