<!--- cfcatch type matching: closest class ancestor (was BUGS.md #24) --->
1:
<cftry>
  <cftry>
    <cfset x = 1 / 0>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
    <cfcatch type="expression"><cfoutput>[EXPR]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|2:
<cftry>
  <cftry>
    <cfset x = 1 / 0>
    <cfcatch type="application"><cfoutput>[APP]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
    <cfcatch type="expression"><cfoutput>[EXPR]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|3:
<cftry>
  <cftry>
    <cfset x = 1 / 0>
    <cfcatch type="application"><cfoutput>[APP]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|4:
<cftry>
  <cftry>
    <cfthrow type="myType" message="boom">
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
    <cfcatch type="myType"><cfoutput>[MYTYPE]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|5:
<cftry>
  <cftry>
    <cfthrow type="myType" message="boom">
    <cfcatch type="expression"><cfoutput>[EXPR]</cfoutput></cfcatch>
    <cfcatch type="application"><cfoutput>[APP]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|6:
<cftry>
  <cftry>
    <cfthrow type="database" message="boom">
    <cfcatch type="any"><cfoutput>[#cfcatch.type#]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|7:
<cftry>
  <cftry>
    <cfthrow type="database" message="boom">
    <cfcatch type="database"><cfoutput>[DB]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|8:
<cftry>
  <cftry>
    <cfthrow type="database" message="boom">
    <cfcatch type="application"><cfoutput>[APP]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|9:
<cftry>
  <cftry>
    <cfthrow type="database" message="boom">
    <cfcatch type="expression"><cfoutput>[EXPR]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|10:
<cftry>
  <cftry>
    <cfthrow type="expression" message="boom">
    <cfcatch type="any"><cfoutput>[#cfcatch.type#]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|11:
<cftry>
  <cftry>
    <cfthrow type="expression" message="boom">
    <cfcatch type="expression"><cfoutput>[EXPR]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|12:
<cftry>
  <cftry>
    <cfthrow type="expression" message="boom">
    <cfcatch type="application"><cfoutput>[APP]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|13:
<cftry>
  <cftry>
    <cfthrow type="my.custom.type" message="boom">
    <cfcatch type="my.custom"><cfoutput>[MC]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|14:
<cftry>
  <cftry>
    <cfthrow type="my.custom.type" message="boom">
    <cfcatch type="my"><cfoutput>[M]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|15:
<cftry>
  <cftry>
    <cfthrow type="my.custom.type" message="boom">
    <cfcatch type="any"><cfoutput>[#cfcatch.type#]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|16:
<cftry>
  <cftry>
    <cfinclude template="nonexistent_file.cfm">
    <cfcatch type="any"><cfoutput>[#cfcatch.type#]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|17:
<cftry>
  <cftry>
    <cfinclude template="nonexistent_file.cfm">
    <cfcatch type="missinginclude"><cfoutput>[MI]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|18:
<cftry>
  <cftry>
    <cfinclude template="nonexistent_file.cfm">
    <cfcatch type="template"><cfoutput>[TPL]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
|19:
<cftry>
  <cftry>
    <cfinclude template="nonexistent_file.cfm">
    <cfcatch type="expression"><cfoutput>[EXPR]</cfoutput></cfcatch>
    <cfcatch type="any"><cfoutput>[ANY]</cfoutput></cfcatch>
  </cftry>
  <cfcatch type="any"><cfoutput>[ERR]</cfoutput></cfcatch>
</cftry>
