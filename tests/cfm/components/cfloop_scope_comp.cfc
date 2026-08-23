<cfcomponent>
	<cffunction name="listVar" returntype="string">
		<cfset var node = "" />
		<cfset var out = "" />
		<cfloop list="/generalSettings/system" index="node" delimiters="/">
			<cfset out = out & "[" & node & "]" />
		</cfloop>
		<cfreturn out & "|after=" & node & "|hasVars=" & structKeyExists(variables,"node") & "|hasLocal=" & structKeyExists(local,"node") />
	</cffunction>

	<cffunction name="listNonVar" returntype="string">
		<cfset var out = "" />
		<cfloop list="/x/y/z" index="idx2" delimiters="/">
			<cfset out = out & "{" & idx2 & "}" />
		</cfloop>
		<cfreturn out & "|after=" & idx2 & "|hasVars=" & structKeyExists(variables,"idx2") & "|hasLocal=" & structKeyExists(local,"idx2") />
	</cffunction>

	<cffunction name="numericVar" returntype="string">
		<cfset var i = 0 />
		<cfset var out = "" />
		<cfloop from="1" to="3" index="i">
			<cfset out = out & "[" & i & "]" />
		</cfloop>
		<cfreturn out & "|after=" & i />
	</cffunction>

	<cffunction name="numericNonVar" returntype="string">
		<cfset var out = "" />
		<cfloop from="1" to="3" index="idx9">
			<cfset out = out & "{" & idx9 & "}" />
		</cfloop>
		<cfreturn out & "|after=" & idx9 />
	</cffunction>

	<cffunction name="arrayVar" returntype="string">
		<cfset var item = "" />
		<cfset var out = "" />
		<cfset var a = ["aa","bb"] />
		<cfloop array="#a#" index="item">
			<cfset out = out & "(" & item & ")" />
		</cfloop>
		<cfreturn out & "|after=" & item />
	</cffunction>

	<cffunction name="collectionVar" returntype="string">
		<cfset var key = "" />
		<cfset var out = "" />
		<cfset var s = structNew() />
		<cfset s.single = "only" />
		<cfloop collection="#s#" item="key">
			<cfset out = out & "[" & ucase(key) & "=" & s[key] & "]" />
		</cfloop>
		<cfreturn out & "|after=" & ucase(key) />
	</cffunction>

	<cffunction name="scriptForInVar" returntype="string">
		<cfscript>
			var out = "";
			var arr = ["p","q","r"];
			for (it1 in arr) {
				out &= "[" & it1 & "]";
			}
			return out & "|after=" & it1;
		</cfscript>
	</cffunction>

	<cffunction name="scriptForInNonVar" returntype="string">
		<cfscript>
			var out = "";
			var arr = ["m","n"];
			for (it2 in arr) {
				out &= "{" & it2 & "}";
			}
			return out & "|after=" & it2;
		</cfscript>
	</cffunction>
</cfcomponent>
