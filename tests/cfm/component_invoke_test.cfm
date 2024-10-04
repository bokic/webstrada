<cfoutput>START;</cfoutput>
<cfobject type="component" name="o" component="components/person">
<cfset o.init("Pat")>
<cfoutput>#o.getName()#|</cfoutput>
<cfinvoke component="#o#" method="setName" argumentcollection="#{value:'Quinn'}#">
<cfinvoke component="#o#" method="getName" returnvariable="r">
<cfoutput>#r#|</cfoutput>
<cfinvoke component="components/person" method="init" argumentcollection="#{name:'Rex'}#" returnvariable="rr">
<cfoutput>#rr.getName()#|</cfoutput>
