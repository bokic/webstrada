<cfimport prefix="mytag" taglib="customtags">
<cfset s = {a: "collA", c: "collC"}>
<cfmodule template="customtags/attrcoll.cfm" a="explicitA" b="explicitB" attributecollection="#s#">
