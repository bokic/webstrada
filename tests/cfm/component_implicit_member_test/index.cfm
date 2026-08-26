<cfset holder = CreateObject("component", "ImplicitMemberHolder")>
<cfoutput>#holder.value()#|#IsStruct(holder.mappings)#</cfoutput>
