<cfimport prefix="mytag" taglib="customtags">
<cfoutput>[TEST1]</cfoutput>
<mytag:gtdata_pages count="3"><mytag:gtdata_page /></mytag:gtdata_pages>
<cfoutput>[TEST2]</cfoutput>
<mytag:gtdata_pages from="2" count="4"><mytag:gtdata_page /></mytag:gtdata_pages>
<cfoutput>[TEST3]</cfoutput>
<mytag:gtdata_page />
<cfoutput>[DONE]</cfoutput>
