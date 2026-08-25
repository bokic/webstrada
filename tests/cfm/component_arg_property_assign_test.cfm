<cfoutput>START;</cfoutput>
<cfset event = CreateObject("component", "components.proptest.Event")>
<cfset event.name = "beforeHtmlHeadEnd" />
<cfset plugin = CreateObject("component", "components.proptest.Proc")>
<cfset result = plugin.processEvent(event) />
<cfoutput>#result.getOutputData()#|</cfoutput>
<cfset event2 = CreateObject("component", "components.proptest.Event")>
<cfset event2.name = "otherEvent" />
<cfset result2 = plugin.processEvent(event2) />
<cfoutput>#result2.getOutputData()#|</cfoutput>
<cfset event3 = CreateObject("component", "components.proptest.Event")>
<cfset event3.name = "beforeHtmlHeadEnd" />
<cfset plugin3 = CreateObject("component", "components.proptest.Proc")>
<cfset r3 = plugin3.processEvent(event3) />
<cfoutput>#r3.getOutputData()#|</cfoutput>
<!--- deep path into a CFC: e.some.deep = v auto-creates the path --->
<cfset e5 = CreateObject("component", "components.proptest.Event")>
<cfset e5.some.deep = "v">
<cfoutput>#e5.some.deep#|</cfoutput>
<!--- single missing member on a CFC is auto-created in the this scope --->
<cfset e6 = CreateObject("component", "components.proptest.Event")>
<cfset e6.some = "scalar">
<cfoutput>#e6.some#|</cfoutput>
<!--- nested path through a component argument --->
<cfset e7 = CreateObject("component", "components.proptest.Event")>
<cfset e7.name = "beforeHtmlHeadEnd" />
<cfset e7.nested.inner = "deepval">
<cfoutput>#e7.nested.inner#|</cfoutput>
<cfoutput>END</cfoutput>
