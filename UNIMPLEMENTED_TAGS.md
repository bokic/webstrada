# Unimplemented CFML Tags

Direct custom-tag syntax (`<cf_name>`) is implemented; this does not change the status of the separate built-in `<cflayout>` tag.

Total: 89 tags (53.94% of 165) marked `❌ No` in PROGRESS.md (CFML Tags table), all of which are planned to be implemented eventually.

Compatibility note (2026-08-27): `<cfmail>`, `<cfmailpart>`, and `<cfmailparam>`
are accepted as non-delivering logging stubs; no SMTP, multipart, or
attachment operation is performed.

Compatibility note (2026-08-27): script-form `savecontent` and `include`
statements are supported by the compiler; this is separate from the remaining
tag inventory and fixes script CFC/plugin loading.

| Category | Count | % of total | Tags |
|---|---|---|---|
| **UI/Form controls** | 29 | 17.58% | cfapplet, cfcalendar, cfcol, cfform, cfformgroup, cfformitem, cfgrid, cfgridcolumn, cfgridrow, cfgridupdate, cfinput, cflayout, cflayoutarea, cfmediaplayer, cfmenu, cfmenuitem, cfmessagebox, cfpod, cfprogressbar, cfselect, cfslider, cftable, cftextarea, cftextinput, cftooltip, cftree, cftreeitem, cfwindow, cffileupload |
| **Document/PDF/Report** | 16 | 9.70% | cfdocument, cfdocumentitem, cfdocumentsection, cfhtmltopdf, cfhtmltopdfitem, cfpdf, cfpdfform, cfpdfformparam, cfpdfparam, cfpdfsubform, cfpresentation, cfpresentationslide, cfpresenter, cfprint, cfreport, cfreportparam |
| **Web/HTTP/Output** | 1 | 0.61% | cfwebsocket |
| **Mail/Network** | 7 | 4.24% | cfimap, cfimapfilter, cfldap, cfmail, cfmailparam, cfmailpart, cfpop |
| **Exchange** | 8 | 4.85% | cfexchangecalendar, cfexchangeconnection, cfexchangecontact, cfexchangeconversation, cfexchangefilter, cfexchangefolder, cfexchangemail, cfexchangetask |
| **Misc/Integration** | 6 | 3.64% | cfmap, cfmapitem, cfregistry, cfservlet, cfservletparam, cfsharepoint |
| **Security/Auth** | 4 | 2.42% | cfauthenticate, cfimpersonate, cfNTauthenticate, cfoauth |
| **Chart/Graph** | 6 | 3.64% | cfchart, cfchartdata, cfchartseries, cfchartset, cfgraph, cfgraphdata |
| **AJAX** | 5 | 3.03% | cfajaximport, cfajaxproxy, cfclient, cfclientsettings, cfsprydataset |
| **Threading/Concurrency** | 2 | 1.21% | cflock, cfthread |
| **Search** | 3 | 1.82% | cfcollection, cfindex, cfsearch |
| **Spreadsheet*** | 1 | 0.61% | cfspreadsheet |
| **Java/.NET objects** | 1 | 0.61% | cfjava |

Note: the **Component/CFC** category (cfassociate, cfimport, cfinterface, cfinvokeargument, cfmodule) is fully implemented. On 2026-08-25 the custom-tag runtime landed: `<cfimport taglib/prefix>`, `<prefix:tag>` invocations (single/pair/self-closing), `<cfmodule>`, `<cfassociate>`, the `thisTag`/`attributes`/`caller` scopes, `cfexit` custom-tag semantics, and `GetBaseTagData`/`GetBaseTagList` are all implemented and byte-verified against CF 2025 (see PROGRESS.md "Custom tags" section + tests/cfm/custom_tag_*_test.cfm). The **File/Directory/Zip** category (cfdirectory, cffile, cfzip, cfzipparam) was implemented on 2026-08-09 and all four are byte-verified against CF 2025 (cfzip/cfzipparam after the CF `zip` package was installed on the RDS host — see BUGS_CF.md). The **Application/Scope** category (cfparam, cfobjectcache) was implemented on 2026-08-09 and both are byte-verified against CF 2025 (see PROGRESS.md). The **Web/HTTP/Output** category (cfcookie, cfhtmlhead, cfprocessingdirective, cfsavecontent, cfsetting) was implemented on 2026-08-10 and all five are byte-verified against CF 2025; cfwebsocket remains unimplemented (see PROGRESS.md). cfhtmlhead is written to the response head (verified byte-for-byte); cfcookie's Set-Cookie bytes are pinned by the `WebOutputTest.Cookie*` unit tests because the byte-verifier only compares response bodies. On 2026-08-10 cfexecute (Misc/Integration) and cffeed + cfwddx (XML/WDDX/Feed) were implemented and all three are byte-verified against CF 2025 (tests/cfm/cfexecute_test.cfm, cfwddx_test.cfm, cffeed_test.cfm). cffeed was unblocked for byte-verification the same day once the CF `feed` package was installed on the RDS host (see BUGS_CF.md). The **Security/Auth** login tags (cflogin, cfloginuser, cflogout) were implemented on 2026-08-11 with the auth functions (GetAuthUser, GetUserRoles, IsUserLoggedIn, IsUserInRole, IsUserInAnyRole) and are byte-verified against CF 2025 (tests/cfm/cflogin_basic_test.cfm + cflogin_user_outside_test.cfm + cflogout_test.cfm + cflogin_roles_test.cfm + cflogin_timing_test.cfm + cflogin_session_storage_test.cfm; the Set-Cookie bytes and cookie/session round-trips are pinned by the `LoginTagTest` unit tests). On 2026-08-24 cfftp (Mail/Network) and cfschedule (Misc/Integration) became **log-only stubs**: they compile with CF's attribute validation (unknown attributes and a missing `action` are compile-time errors) and the runtime just logs the call to the engine log on stderr — no FTP/scheduling is performed (see PROGRESS.md; tests/cfm/cfftp_stub_test.cfm + cfschedule_stub_test.cfm).
# Compatibility note (2026-08-27)

`<cfmail>` is accepted as a non-delivering logging stub. Its evaluated
attributes are logged and its body is consumed without SMTP delivery.
