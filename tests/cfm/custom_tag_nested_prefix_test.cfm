<cfimport prefix="mytag" taglib="customtags">
<cfset callervar = "outer_val">
<mytag:nested_pref>BODY</mytag:nested_pref>
<cfoutput>[AFTER:#callervar#]</cfoutput>
