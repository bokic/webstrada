<!--- Tier-1: GetFunctionList membership checks (verified against CF 2025; the
     engine's registry differs from CF's 800-entry Java method list, so only
     stable memberships that agree are asserted). --->
<cfset fl = GetFunctionList()>
<cfoutput>
1:[#StructKeyExists(fl, "GETTOKEN")#]|2:[#StructKeyExists(fl, "GETLOCALHOSTIP")#]|3:[#StructKeyExists(fl, "OBJECTEQUALS")#]|4:[#StructKeyExists(fl, "GETTEMPLATEPATH")#]|5:[#StructKeyExists(fl, "PARAMETEREXISTS")#]|6:[#StructKeyExists(fl, "LOCATION")#]|7:[#StructKeyExists(fl, "SETVARIABLE")#]|8:[#StructKeyExists(fl, "ISDDX")#]|9:[#StructKeyExists(fl, "GETCSPNONCE")#]|10:[#StructKeyExists(fl, "TRANSACTIONCOMMIT")#]
</cfoutput>
