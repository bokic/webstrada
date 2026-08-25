<cfcomponent>
	<cffunction name="run" access="public" returntype="string">
		<cfset var i = 0 />
		<cfset var data = xmlParse('<plugin><listens><event name="SubscriptionHandler" type="x" priority="1" /></listens></plugin>') />
		<cfset var out = "" />
		<cfloop index="i" from="1" to="#arrayLen(data.plugin.listens.xmlChildren)#">
			<cfset out = listAppend(out, data.plugin.listens.xmlChildren[i].xmlAttributes["name"]) />
		</cfloop>
		<cfreturn out />
	</cffunction>
</cfcomponent>
