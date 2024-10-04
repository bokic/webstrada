# Unimplemented CFML Tags

Total: 91 tags (55.15% of 165) marked `❌ No` in PROGRESS.md (CFML Tags table), all of which are planned to be implemented eventually.

| Category | Count | % of total | Tags |
|---|---|---|---|
| **UI/Form controls** | 29 | 17.58% | cfapplet, cfcalendar, cfcol, cfform, cfformgroup, cfformitem, cfgrid, cfgridcolumn, cfgridrow, cfgridupdate, cfinput, cflayout, cflayoutarea, cfmediaplayer, cfmenu, cfmenuitem, cfmessagebox, cfpod, cfprogressbar, cfselect, cfslider, cftable, cftextarea, cftextinput, cftooltip, cftree, cftreeitem, cfwindow, cffileupload |
| **Document/PDF/Report** | 16 | 9.70% | cfdocument, cfdocumentitem, cfdocumentsection, cfhtmltopdf, cfhtmltopdfitem, cfpdf, cfpdfform, cfpdfformparam, cfpdfparam, cfpdfsubform, cfpresentation, cfpresentationslide, cfpresenter, cfprint, cfreport, cfreportparam |
| **Web/HTTP/Output** | 1 | 0.61% | cfwebsocket |
| **Mail/Network** | 8 | 4.85% | cfftp, cfimap, cfimapfilter, cfldap, cfmail, cfmailparam, cfmailpart, cfpop |
| **Exchange** | 8 | 4.85% | cfexchangecalendar, cfexchangeconnection, cfexchangecontact, cfexchangeconversation, cfexchangefilter, cfexchangefolder, cfexchangemail, cfexchangetask |
| **Misc/Integration** | 7 | 4.24% | cfmap, cfmapitem, cfregistry, cfschedule, cfservlet, cfservletparam, cfsharepoint |
| **Security/Auth** | 4 | 2.42% | cfauthenticate, cfimpersonate, cfNTauthenticate, cfoauth |
| **Chart/Graph** | 6 | 3.64% | cfchart, cfchartdata, cfchartseries, cfchartset, cfgraph, cfgraphdata |
| **AJAX** | 5 | 3.03% | cfajaximport, cfajaxproxy, cfclient, cfclientsettings, cfsprydataset |
| **Threading/Concurrency** | 2 | 1.21% | cflock, cfthread |
| **Search** | 3 | 1.82% | cfcollection, cfindex, cfsearch |
| **Spreadsheet*** | 1 | 0.61% | cfspreadsheet |
| **Java/.NET objects** | 1 | 0.61% | cfjava |

Note: the **Component/CFC** category (cfassociate, cfimport, cfinterface, cfinvokeargument, cfmodule) is fully implemented (2026-08-09). cfassociate, cfmodule and the cfimport `taglib`/`prefix` forms depend on the custom-tag runtime, which this engine does not implement; they throw a catchable `Application` "not supported / custom tags are not implemented" error (see PROGRESS.md). The **File/Directory/Zip** category (cfdirectory, cffile, cfzip, cfzipparam) was implemented on 2026-08-09 and all four are byte-verified against CF 2025 (cfzip/cfzipparam after the CF `zip` package was installed on the RDS host — see BUGS_CF.md). The **Application/Scope** category (cfparam, cfobjectcache) was implemented on 2026-08-09 and both are byte-verified against CF 2025 (see PROGRESS.md). The **Web/HTTP/Output** category (cfcookie, cfhtmlhead, cfprocessingdirective, cfsavecontent, cfsetting) was implemented on 2026-08-10 and all five are byte-verified against CF 2025; cfwebsocket remains unimplemented (see PROGRESS.md). cfhtmlhead is written to the response head (verified byte-for-byte); cfcookie's Set-Cookie bytes are pinned by the `WebOutputTest.Cookie*` unit tests because the byte-verifier only compares response bodies. On 2026-08-10 cfexecute (Misc/Integration) and cffeed + cfwddx (XML/WDDX/Feed) were implemented and all three are byte-verified against CF 2025 (tests/cfm/cfexecute_test.cfm, cfwddx_test.cfm, cffeed_test.cfm). cffeed was unblocked for byte-verification the same day once the CF `feed` package was installed on the RDS host (see BUGS_CF.md). The **Security/Auth** login tags (cflogin, cfloginuser, cflogout) were implemented on 2026-08-11 with the auth functions (GetAuthUser, GetUserRoles, IsUserLoggedIn, IsUserInRole, IsUserInAnyRole) and are byte-verified against CF 2025 (tests/cfm/cflogin_basic_test.cfm + cflogin_user_outside_test.cfm + cflogout_test.cfm + cflogin_roles_test.cfm + cflogin_timing_test.cfm + cflogin_session_storage_test.cfm; the Set-Cookie bytes and cookie/session round-trips are pinned by the `LoginTagTest` unit tests).
