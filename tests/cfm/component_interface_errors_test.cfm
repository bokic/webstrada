<cfoutput>START|</cfoutput>
<cfloop list="iface_bad_missing,iface_bad_private,iface_bad_rettype,iface_bad_required,iface_bad_paramtype,iface_bad_default,iface_bad_argname,iface_bad_noiface,iface_implcomp,iface_extcomp" index="comp">
<cftry>
  <cfset x = CreateObject("component", "components/#comp#")>
  <cfoutput>[#comp#:NO_ERROR]</cfoutput>
<cfcatch type="any">
  <cfoutput>[#comp#:#cfcatch.type#|#cfcatch.message#|#cfcatch.detail#]</cfoutput>
</cfcatch>
</cftry>
</cfloop>
