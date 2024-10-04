<cfoutput>START;</cfoutput>
<cfset child = new components.super_child()>
<cfoutput>#child.greet()#|</cfoutput>
<cfoutput>#child.getValue()#|</cfoutput>
