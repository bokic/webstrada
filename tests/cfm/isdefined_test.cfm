<cfapplication name="isdefined_test" sessionmanagement="true">
<cfset x = 5>
<cfset pagevar = "pv">
<cfset s = {a: 1, b: 2}>
<cfset s.a = 10>
<cfset s.d = {e: 99}>
<cfset r = {}><cfset r.q = "zzz">
<cfset session.sv = 7>
<cfset application.av = 8>
<cfset url.uv = 3>
<cfset form.fv = 4>
<cfset q = queryNew("id,name","integer,varchar")>
<cfset queryAddRow(q)>
<cfset querySetCell(q, "id", 1)>
<cfset querySetCell(q, "name", "alice")>
<cfoutput>
basic_defined:#IsDefined("x")#|
basic_undefined:#IsDefined("nosuchvar")#|
struct_key:#IsDefined("s.a")#|
struct_missing_key:#IsDefined("s.nokey")#|
struct_root:#IsDefined("s")#|
struct_scalar_member:#IsDefined("s.a.b")#|
struct_nested:#IsDefined("s.d.e")#|
struct_nested_root:#IsDefined("s.d")#|
struct_nested_missing:#IsDefined("s.d.x")#|
struct_scalar_member2:#IsDefined("x.y")#|
query_root:#IsDefined("q")#|
query_column:#IsDefined("q.name")#|
query_missing_column:#IsDefined("q.nope")#|
query_column2:#IsDefined("q.id")#|
session_defined:#IsDefined("session.sv")#|
session_undefined:#IsDefined("session.none")#|
session_case:#IsDefined("SESSION.sv")#|
application_defined:#IsDefined("application.av")#|
application_undefined:#IsDefined("application.NOPE")#|
url_defined:#IsDefined("url.uv")#|
url_undefined:#IsDefined("url.zzz")#|
form_defined:#IsDefined("form.fv")#|
form_undefined:#IsDefined("form.x")#|
cgi_real:#IsDefined("cgi.PATH_INFO")#|
cgi_quirk:#IsDefined("cgi.NOPE")#|
cgi_quirk2:#IsDefined("cgi.NOPE.SUB")#|
variables_defined:#IsDefined("variables.x")#|
variables_undefined:#IsDefined("variables.nosuch")#|
server_root:#IsDefined("server")#|
server_undefined:#IsDefined("server.NOPE")#|
client_undefined:#IsDefined("client.x")#|
this_page:#IsDefined("THIS.x")#|
local_page:#IsDefined("LOCAL.x")#|
local_bare_page:#IsDefined("local")#|
arguments_page:#IsDefined("arguments")#|
empty:#IsDefined("")#|
numeric_arg:#IsDefined(123)#|
scope_server:#IsDefined("server")#|
scope_cgi:#IsDefined("cgi")#|
scope_session:#IsDefined("session")#|
scope_application:#IsDefined("application")#|
scope_form:#IsDefined("form")#|
scope_url:#IsDefined("url")#|
scope_variables:#IsDefined("variables")#|
scope_request:#IsDefined("request")#|
scope_request_missing:#IsDefined("request.rv")#|
request_set:#IsDefined("request.rv2")#|
</cfoutput>
<cfset request.rv2 = 5>
<cfoutput>
request_set2:#IsDefined("request.rv2")#|
request_read:#request.rv2#|
</cfoutput>
<cfscript>
function isdef_udf(a) {
    local.lv = 42;
    var res = "";
    res &= "udf_arguments_named:#IsDefined('arguments.a')#|";
    res &= "udf_arguments_missing:#IsDefined('arguments.b')#|";
    res &= "udf_unqualified_param:#IsDefined('a')#|";
    res &= "udf_local_key:#IsDefined('local.lv')#|";
    res &= "udf_local_bare:#IsDefined('local')#|";
    res &= "udf_local_missing:#IsDefined('local.nope')#|";
    res &= "udf_unqualified_local:#IsDefined('lv')#|";
    res &= "udf_arguments_bare:#IsDefined('arguments')#|";
    res &= "udf_udfname:#IsDefined('isdef_udf')#|";
    res &= "udf_parent_unqualified:#IsDefined('x')#|";
    res &= "udf_parent_unqualified2:#IsDefined('pagevar')#|";
    return res;
}
</cfscript>
<cfoutput>
#isdef_udf("hello")#
after_udf_local:#IsDefined("local")#|
after_udf_arguments:#IsDefined("arguments")#|
</cfoutput>

