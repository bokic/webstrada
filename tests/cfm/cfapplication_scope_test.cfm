<cfapplication name="scope_test_app" sessionmanagement="true" applicationtimeout="#CreateTimeSpan(1,2,3,4)#" sessiontimeout="#CreateTimeSpan(0,0,30,0)#">
<cfset application.counter = 0>
<cfset session.user = "boris">
<cfoutput>
APP=#application.counter#
SESS=#session.user#
META_NAME=#GetApplicationMetadata().NAME#
META_AT=#GetApplicationMetadata().APPLICATIONTIMEOUT#
META_ST=#GetApplicationMetadata().SESSIONTIMEOUT#
META_SM=#GetApplicationMetadata().SESSIONMANAGEMENT#
META_CC=#GetApplicationMetadata().SETCLIENTCOOKIES#
SESMETA=#StructKeyExists(SessionGetMetadata(), "STARTTIME")#
URLPREFIX=#Left(urlSessionFormat("page.cfm"), 13)#
</cfoutput>
