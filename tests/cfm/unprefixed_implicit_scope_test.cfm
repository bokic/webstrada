<!--- Unqualified (unprefixed) name lookup must NOT search implicit scopes
(form/url/cgi/cookie) nor server/session/application, matching ColdFusion's
default `searchimplicitscopes=false`. Only variables-scope names resolve
unprefixed. With `<cfapplication searchimplicitscopes="true">` the implicit
scopes ARE searched (in CF order CGI, URL, FORM, COOKIE). Verified against
CF 2025. --->
<cfapplication searchimplicitscopes="true">
<cfset form.ONLYFORM = "formonly">
<cfset url.ONLYURL = "urlonly">
<cfset url.DUP = "fromurl">
<cfset form.DUP = "fromform">
<cfset variables.ONLYVAR = "varonly">
<cfoutput>#DUP#</cfoutput>|
<cfoutput>#ONLYFORM#</cfoutput>|
<cfoutput>#ONLYURL#</cfoutput>|
<cfoutput>#ONLYVAR#</cfoutput>|
<cfoutput>#form.ONLYFORM#|#url.ONLYURL#|#variables.ONLYVAR#</cfoutput>
