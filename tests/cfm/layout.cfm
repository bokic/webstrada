<cfif thisTag.executionMode eq "start">[START:<cfoutput>#attributes.title#</cfoutput>]<cfelse>[END:<cfoutput>#thisTag.generatedContent#</cfoutput>]</cfif>
