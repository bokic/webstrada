<cfoutput>
--- Test 1: IsDefined checks ---
isProtected defined: #IsDefined("isProtected")#
isAuthenticated defined: #IsDefined("isAuthenticated")#
isAuthorized defined: #IsDefined("isAuthorized")#

--- Test 2: Unqualified calls without UDF ---
<cftry>
#isProtected()#
<cfcatch type="any">catch isProtected: #cfcatch.type# | #cfcatch.message#</cfcatch>
</cftry>
<cftry>
#isAuthenticated()#
<cfcatch type="any">catch isAuthenticated: #cfcatch.type# | #cfcatch.message#</cfcatch>
</cftry>
<cftry>
#isAuthorized()#
<cfcatch type="any">catch isAuthorized: #cfcatch.type# | #cfcatch.message#</cfcatch>
</cftry>

--- Test 3: Unqualified calls with arguments without UDF ---
<cftry>
#isProtected("arg1", 123)#
<cfcatch type="any">catch isProtected args: #cfcatch.type# | #cfcatch.message#</cfcatch>
</cftry>
<cftry>
#isAuthenticated("user1")#
<cfcatch type="any">catch isAuthenticated args: #cfcatch.type# | #cfcatch.message#</cfcatch>
</cftry>
<cftry>
#isAuthorized("admin", "read")#
<cfcatch type="any">catch isAuthorized args: #cfcatch.type# | #cfcatch.message#</cfcatch>
</cftry>

--- Test 4: Struct keys and structKeyExists ---
<cfset st = { isProtected = "p_val", isAuthenticated = "auth_val", isAuthorized = "z_val" }>
st.isProtected: #st.isProtected#
st.isAuthenticated: #st.isAuthenticated#
st.isAuthorized: #st.isAuthorized#
structKeyExists isProtected: #structKeyExists(st, "isProtected")#
structKeyExists isAuthenticated: #structKeyExists(st, "isAuthenticated")#
structKeyExists isAuthorized: #structKeyExists(st, "isAuthorized")#
</cfoutput>

--- Test 5: Tag-defined UDFs ---
<cffunction name="isProtected">
    <cfargument name="mode" default="default">
    <cfreturn "tag_protected_" & arguments.mode>
</cffunction>
<cffunction name="isAuthenticated">
    <cfargument name="user" default="guest">
    <cfreturn "tag_auth_" & arguments.user>
</cffunction>
<cffunction name="isAuthorized">
    <cfargument name="role" default="none">
    <cfreturn "tag_authz_" & arguments.role>
</cffunction>

<cfoutput>
UDF isProtected(): #isProtected()#
UDF isProtected("secure"): #isProtected("secure")#
UDF isAuthenticated(): #isAuthenticated()#
UDF isAuthenticated("john"): #isAuthenticated("john")#
UDF isAuthorized(): #isAuthorized()#
UDF isAuthorized("admin"): #isAuthorized("admin")#
isProtected defined after UDF: #IsDefined("isProtected")#
isAuthenticated defined after UDF: #IsDefined("isAuthenticated")#
isAuthorized defined after UDF: #IsDefined("isAuthorized")#
</cfoutput>
