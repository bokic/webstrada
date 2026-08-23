<cfoutput>
starts_with_slash:#left(cgi.script_name, 1) eq "/"#|
starts_with_slash_uc:#left(CGI.SCRIPT_NAME, 1) eq "/"#|
len_gt_zero:#len(cgi.script_name) gt 0#|
len_gt_zero_uc:#len(CGI.SCRIPT_NAME) gt 0#|
path_info_defined:#isDefined("cgi.path_info")#|
path_info_starts_with_slash:#left(cgi.path_info, 1) eq "/"#|
request_uri_defined:#isDefined("cgi.request_uri")#|
request_uri_starts_with_slash:#left(cgi.request_uri, 1) eq "/"#|
script_name_defined:#isDefined("cgi.script_name")#
</cfoutput>
