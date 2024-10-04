<!--- output attribute whitespace behaviors --->
<cffunction name="g1" output="true">
    <cfoutput>K</cfoutput>
</cffunction>
<cffunction name="g2">
    <cfoutput>K</cfoutput>
</cffunction>
<cffunction name="g3" output="false">
    <cfoutput>K</cfoutput>
</cffunction>
<cfoutput>W1[#g1()#]W2[#g2()#]W3[#g3()#]</cfoutput>
|
<cffunction name="passPlain" output="true">
PLAINTEXT-INSIDE
    <cfoutput>KEEP</cfoutput>
</cffunction>
<cfoutput>PLAIN[#passPlain()#]</cfoutput>
|
<!--- function inside a cfif is hoisted --->
<cfif 1 IS 1>
<cffunction name="inif" output="false">
    <cfreturn 7>
</cffunction>
</cfif>
<cfoutput>INIF:#inif()#</cfoutput>
|
<!--- body that is only plain text (output true keeps, false drops) --->
<cffunction name="plainOnly" output="true">
ONLY-PLAIN
</cffunction>
<cfoutput>PO[#plainOnly()#]</cfoutput>
|
<!--- whitespace regions between function blocks --->
<cffunction name="f1" output="false"><cfreturn "A"></cffunction>
<cffunction name="f2" output="false"><cfreturn "B"></cffunction>
<cfoutput>#f1()#,#f2()#</cfoutput>
|
<!--- string concatenation with numeric coercion --->
<cffunction name="math" output="false">
    <cfargument name="a" type="numeric" required="true">
    <cfargument name="b" type="numeric" required="true">
    <cfreturn (a + b) * 2>
</cffunction>
<cfoutput>MATH:#math(3, 4)#</cfoutput>
|
<!--- cfreturn inside a loop --->
<cffunction name="loopy" output="false">
    <cfargument name="n" required="true">
    <cfloop index="i" from="1" to="#n#">
        <cfif i EQ 3>
            <cfreturn "hit" & i>
        </cfif>
    </cfloop>
    <cfreturn "none">
</cffunction>
<cfoutput>LOOP:#loopy(5)#,#loopy(2)#</cfoutput>
|
<!--- var-scoped and arguments together --->
<cffunction name="combo" output="false">
    <cfargument name="p">
    <cfargument name="q">
    <cfset var r = p & q>
    <cfreturn r & "|" & StructKeyList(arguments)>
</cffunction>
<cfoutput>COMBO:#combo("x","y")#</cfoutput>
