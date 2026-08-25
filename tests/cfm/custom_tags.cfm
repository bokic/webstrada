<cfimport prefix="mytag" taglib="customtags">
<cfset callervar = "initial_val">
<mytag:simple name="Alice" />
<cfoutput>[AFTER:#callervar#]</cfoutput>

<mytag:wrapper title="Header">Hello World</mytag:wrapper>

<mytag:looper times="3">
<cfoutput>[ITER:#loopCount#]</cfoutput>
</mytag:looper>
