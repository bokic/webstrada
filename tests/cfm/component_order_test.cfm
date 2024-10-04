<cfoutput>START;</cfoutput>
<cfset o = new components.order_comp()>
<cfoutput>#SerializeJSON(o)#|</cfoutput>
<cfoutput>#ListSort(StructKeyList(o), "text")#|</cfoutput>
