<cfoutput>START|</cfoutput>
<cfinvoke component="components/argcalc" method="add" returnvariable="r1">
  <cfinvokeargument name="a" value="3">
  <cfinvokeargument name="b" value="4">
</cfinvoke>
<cfoutput>#r1#|</cfoutput>
<cfinvoke component="components/argcalc" method="greet" returnvariable="r2">
  <cfinvokeargument name="who" value="World">
</cfinvoke>
<cfoutput>#r2#|</cfoutput>
<cfinvoke component="components/argcalc" method="two" returnvariable="r3">
  <cfinvokeargument name="name" value="x">
  <cfinvokeargument name="value" value="#1+1#">
</cfinvoke>
<cfoutput>#r3#|</cfoutput>
<cfinvoke component="components/argcalc" method="two" returnvariable="r4" argumentcollection="#{name:'coll', value:'arg'}#">
  <cfinvokeargument name="value" value="child">
</cfinvoke>
<cfoutput>#r4#|</cfoutput>
<cfinvoke component="components/argcalc" method="add" returnvariable="r5" argumentcollection="#{b:5, a:7}#">
</cfinvoke>
<cfoutput>#r5#|</cfoutput>
<cfinvoke component="components/argcalc" method="add" returnvariable="r6" argumentcollection="#{x:5, a:7}#">
</cfinvoke>
<cfoutput>#r6#|</cfoutput>
<cffunction name="udf_add" output="false">
  <cfargument name="a" type="numeric" required="true">
  <cfargument name="b" type="numeric" required="true">
  <cfreturn arguments.a + arguments.b>
</cffunction>
<cfinvoke method="udf_add" returnvariable="r7">
  <cfinvokeargument name="a" value="20">
  <cfinvokeargument name="b" value="22">
</cfinvoke>
<cfoutput>#r7#|</cfoutput>
<cfinvoke method="udf_add" returnvariable="r8" argumentcollection="#{a:5, b:6}#">
</cfinvoke>
<cfoutput>#r8#</cfoutput>
