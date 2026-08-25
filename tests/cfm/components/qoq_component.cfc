<cfcomponent>
<cffunction name="getCategories" output="false">
    <cfargument name="adminMode" type="boolean" required="true">
    <cfset var categoriesQuery = QueryNew("id,name,post_count", "integer,varchar,integer")>
    <cfset var postsQuery = QueryNew("id,category_id", "integer,integer")>
    <cfset QueryAddRow(categoriesQuery)><cfset QuerySetCell(categoriesQuery, "id", 1)><cfset QuerySetCell(categoriesQuery, "name", "catA")><cfset QuerySetCell(categoriesQuery, "post_count", 2)>
    <cfset QueryAddRow(postsQuery)><cfset QuerySetCell(postsQuery, "id", 10)><cfset QuerySetCell(postsQuery, "category_id", 1)>
    <cfquery name="categories" dbtype="query">
        SELECT    *
        FROM categoriesQuery, postsQuery
        WHERE categoriesQuery.id = postsQuery.category_id
        <cfif NOT arguments.adminMode>
            AND post_count > 0
        </cfif>
        ORDER BY name
    </cfquery>
    <cfreturn categories.recordcount & ":" & categories.name>
</cffunction>
</cfcomponent>
