<cfscript>
// 1. Without _cf_clientid: should fail with Application exception
try {
    verifyClient();
    writeOutput("1:[FAIL: no exception]");
} catch (Application e) {
    writeOutput("1:[OK:" & e.message & "|" & e.detail & "]");
}

// 2. No session, but URL._cf_clientid provided: should succeed
url._cf_clientid = "ANYID";
try {
    verifyClient();
    writeOutput("2:[OK]");
} catch (any e) {
    writeOutput("2:[FAIL:" & e.message & "]");
}
structDelete(url, "_cf_clientid");

// 3. No session, but FORM._cf_clientid provided: should succeed
form._cf_clientid = "ANYID";
try {
    verifyClient();
    writeOutput("3:[OK]");
} catch (any e) {
    writeOutput("3:[FAIL:" & e.message & "]");
}
structDelete(form, "_cf_clientid");
</cfscript>

<!--- 4. With session enabled --->
<cfapplication name="vc_test_app" sessionmanagement="yes">
<cfscript>
// Session is now enabled; expected clientid is UCase(Hash(session.urltoken, "MD5"))
expectedId = uCase(hash(session.urltoken, "MD5"));

// 4a. Mismatched client ID: should fail
url._cf_clientid = "INVALID_HASH_VALUE";
try {
    verifyClient();
    writeOutput("4a:[FAIL: no exception]");
} catch (Application e) {
    writeOutput("4a:[OK:" & e.message & "|" & e.detail & "]");
}

// 4b. Matching client ID in URL: should succeed
url._cf_clientid = expectedId;
try {
    verifyClient();
    writeOutput("4b:[OK]");
} catch (any e) {
    writeOutput("4b:[FAIL:" & e.message & "]");
}
structDelete(url, "_cf_clientid");

// 4c. Matching client ID in FORM: should succeed
form._cf_clientid = expectedId;
try {
    verifyClient();
    writeOutput("4c:[OK]");
} catch (any e) {
    writeOutput("4c:[FAIL:" & e.message & "]");
}
structDelete(form, "_cf_clientid");
</cfscript>
