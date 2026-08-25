<cfif thisTag.executionMode eq "start">
[ST]<cfexit method="exittemplate">[ST-AFTER]
<cfelse>
[EN]<cfexit method="exittemplate">[EN-AFTER]
</cfif>
