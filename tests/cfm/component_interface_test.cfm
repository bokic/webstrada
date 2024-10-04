<cfoutput>START|</cfoutput>
<cfset d = CreateObject("component", "components/animal_dog")>
<cfoutput>#d.getSound(3)#|#d.getLegs()#|#IsInstanceOf(d, "iface_animal")#|#IsInstanceOf(d, "components.iface_animal")#|#IsInstanceOf(d, "animal_dog")#|</cfoutput>
<cfset bi = CreateObject("component", "components/iface_baseimpl")>
<cfoutput>#bi.baseMethod()#|#bi.childMethod()#|#IsInstanceOf(bi, "iface_base")#|#IsInstanceOf(bi, "iface_child")#|</cfoutput>
<cfset si = CreateObject("component", "components/iface_script_impl")>
<cfoutput>#si.scriptMethod(1)#|#IsInstanceOf(si, "iface_script")#|</cfoutput>
<cftry>
  <cfset x = CreateObject("component", "components/iface_animal")>
  <cfoutput>NO_ERROR</cfoutput>
<cfcatch type="any">
  <cfoutput>CAUGHT:#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#</cfoutput>
</cfcatch>
</cftry>
