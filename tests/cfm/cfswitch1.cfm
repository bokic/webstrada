<cfset fruit = "apple" />
<cfset color = "blue" />
<cfoutput><cfswitch expression="#fruit#"><cfcase value="banana">yellow</cfcase><cfcase value="apple">red</cfcase><cfdefaultcase>unknown</cfdefaultcase></cfswitch>|<cfswitch expression="#color#"><cfcase value="red">warm</cfcase><cfcase value="green">cool</cfcase><cfdefaultcase>other</cfdefaultcase></cfswitch></cfoutput>
