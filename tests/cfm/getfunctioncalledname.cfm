<cffunction name="testFunc" returntype="string">
    <cfreturn GetFunctionCalledName()>
</cffunction>

<cffunction name="innerFunc" returntype="string">
    <cfreturn GetFunctionCalledName()>
</cffunction>

<cffunction name="outerFunc" returntype="string">
    <cfset var before = GetFunctionCalledName()>
    <cfset var in_inner = innerFunc()>
    <cfset var after = GetFunctionCalledName()>
    <cfreturn before & "|" & in_inner & "|" & after>
</cffunction>

<cffunction name="countDown" returntype="string">
    <cfargument name="n" type="numeric" required="true">
    <cfif arguments.n LTE 1>
        <cfreturn GetFunctionCalledName() & ":" & arguments.n>
    </cfif>
    <cfreturn GetFunctionCalledName() & ":" & arguments.n & "," & countDown(arguments.n - 1)>
</cffunction>

<cfoutput>
TopLevel: [#GetFunctionCalledName()#]
Direct: [#testFunc()#]
Upper: [#TESTFUNC()#]
Mixed: [#TestFunc()#]

<cfset fnRef = testFunc>
Alias1: [#fnRef()#]
<cfset otherRef = fnRef>
Alias2: [#otherRef()#]

Nested: [#outerFunc()#]
Recursive: [#countDown(3)#]

<cfscript>
    closureFn = function() {
        return GetFunctionCalledName();
    };
    writeOutput("Closure: [" & closureFn() & "]\n");
    closureAlias = closureFn;
    writeOutput("ClosureAlias: [" & closureAlias() & "]\n");

    s = structNew();
    s.memberFunc = testFunc;
    writeOutput("StructMember: [" & s.memberFunc() & "]\n");
    writeOutput("StructMemberUpper: [" & s.MEMBERFUNC() & "]\n");

    comp = createObject("component", "components.gfc_comp");
    writeOutput("CompMethod: [" & comp.getMyName() & "]\n");
    writeOutput("CompMethodUpper: [" & comp.GETMYNAME() & "]\n");
    writeOutput("CompMethodMixed: [" & comp.GetMyName() & "]\n");
    writeOutput("CompBody: [" & comp.getBodyName() & "]\n");

    writeOutput("InvokeUDF: [" & invoke("", "testFunc") & "]\n");
    writeOutput("InvokeUDFUpper: [" & invoke("", "TESTFUNC") & "]\n");
    writeOutput("InvokeComp: [" & invoke(comp, "getMyName") & "]\n");
    writeOutput("InvokeCompUpper: [" & invoke(comp, "GETMYNAME") & "]\n");

    function arrayCb(item) {
        writeOutput(GetFunctionCalledName() & "=" & item & " ");
    }
    writeOutput("ArrayEach: [");
    [1, 2].each(arrayCb);
    writeOutput("]\n");
</cfscript>

<cfinvoke method="testFunc" returnvariable="cinv1" />
CfInvokeUDF: [#cinv1#]
<cfinvoke method="TESTFUNC" returnvariable="cinv2" />
CfInvokeUDFUpper: [#cinv2#]
<cfinvoke component="#comp#" method="getMyName" returnvariable="cinv3" />
CfInvokeComp: [#cinv3#]
<cfinvoke component="#comp#" method="GETMYNAME" returnvariable="cinv4" />
CfInvokeCompUpper: [#cinv4#]
</cfoutput>
