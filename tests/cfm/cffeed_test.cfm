<!--- cffeed: RSS/Atom feed read and create (byte-verified against CF 2025;
     the feed package was installed on the RDS host on 2026-08-10). --->
<cffile action="write" file="#GetTempDirectory()#webstrada_feed_rss.xml" output='<?xml version="1.0"?><rss version="2.0"><channel><title>Test</title><link>http://x</link><description>d</description><language>en-us</language><pubDate>Mon, 06 Jan 2020 12:00:00 GMT</pubDate><generator>gen</generator><ttl>30</ttl><item><title>I1</title><link>http://x/1</link><description type="html">desc</description><pubDate>Mon, 06 Jan 2020 12:00:00 GMT</pubDate><guid isPermaLink="false">g1</guid><author>a@b.c</author><category domain="catdom">catval</category></item></channel></rss>' addnewline="no"></cffile>
<cffeed action="read" source="#GetTempDirectory()#webstrada_feed_rss.xml" name="feed" properties="props" query="q">
<cfoutput>#feed.version#:#feed.title#:#feed.pubDate#:#feed.generator#:#feed.ttl#</cfoutput>
<cfoutput>|I:#feed.item[1].title#:#feed.item[1].description.type#:#feed.item[1].description.value#:#feed.item[1].guid.value#:#feed.item[1].guid.isPermaLink#:#feed.item[1].pubDate#:#feed.item[1].author#:#feed.item[1].category[1].value#</cfoutput>
<cfoutput>|P:#props.title#:#props.pubDate#</cfoutput>
<cfoutput>|Q:#q.recordcount#:#q.content[1]#:#q.contenttype[1]#:#q.id[1]#:#q.idpermalink[1]#:#q.publisheddate[1]#:#q.authoremail[1]#</cfoutput>

<cffile action="write" file="#GetTempDirectory()#webstrada_feed_atom.xml" output='<?xml version="1.0"?><feed xmlns="http://www.w3.org/2005/Atom"><title>ATest</title><id>urn:1</id><updated>2020-01-06T12:00:00Z</updated><author><name>Auth</name></author><entry><title type="html">E1</title><id>urn:2</id><updated>2020-01-06T12:00:00Z</updated><published>2020-01-06T12:00:00Z</published><summary>sum</summary><content type="html">body</content><author><name>EAuth</name></author></entry></feed>' addnewline="no"></cffile>
<cffeed action="read" source="#GetTempDirectory()#webstrada_feed_atom.xml" name="feed2" properties="props2" query="q2">
<cfoutput>|A:#feed2.version#:#feed2.title.value#:#feed2.author[1].name#:#feed2.updated#</cfoutput>
<cfoutput>|E:#feed2.entry[1].title.value#:#feed2.entry[1].title.type#:#feed2.entry[1].summary.value#:#feed2.entry[1].content[1].value#:#feed2.entry[1].content[1].type#:#feed2.entry[1].updated#:#feed2.entry[1].author[1].name#</cfoutput>
<cfoutput>|P2:#props2.title.value#</cfoutput>
<cfoutput>|Q2:#q2.recordcount#:#q2.title[1]#:#q2.titletype[1]#:#q2.updateddate[1]#:#q2.content[1]#:#q2.contenttype[1]#:#q2.summary[1]#:#q2.authorname[1]#</cfoutput>

<cfset crss = {version="rss_2.0", title="My Feed", link="http://example.com", description="A test feed", language="en-us", encoding="UTF-8"}>
<cfset crss.item = [{title="First", link="http://example.com/1", description={value="Desc 1", type="text"}, pubDate="Mon, 06 Jan 2020 12:00:00 GMT", author="a@b.c", comments="http://c", guid={value="g1", isPermaLink="false"}}]>
<cffeed action="create" name="#crss#" xmlvar="xrss">
<cfoutput>|CRSS:#Replace(xrss, "#chr(13)##chr(10)#", "|", "ALL")#</cfoutput>

<cfset catom = {version="atom_1.0", title={type="text", value="ATitle"}, id="urn:1", updated="2020-01-06T12:00:00Z", subtitle={type="text", value="sub"}, rights="(c)", encoding="UTF-8"}>
<cfset catom.entry = [{title={type="text", value="EOne"}, id="urn:2", updated="2020-01-06T12:00:00Z", published="2020-01-06T12:00:00Z", summary={type="text", value="sm"}, content=[{type="html", value="cb"}]}]>
<cffeed action="create" name="#catom#" xmlvar="xatom">
<cfoutput>|CATOM:#Replace(xatom, "#chr(13)##chr(10)#", "|", "ALL")#</cfoutput>

<!--- RSS 1.0 (RDF) read --->
<cffile action="write" file="#GetTempDirectory()#webstrada_feed_rss10.xml" output='<?xml version="1.0"?><rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns##" xmlns="http://purl.org/rss/1.0/"><channel rdf:about="http://x/"><title>R10</title><link>http://x</link><description>d10</description></channel><item rdf:about="http://x/1"><title>I10</title><link>http://x/1</link><description>desc10</description></item></rdf:RDF>' addnewline="no"></cffile>
<cffeed action="read" source="#GetTempDirectory()#webstrada_feed_rss10.xml" name="feed10" query="q10">
<cfoutput>|R10:#feed10.version#:#feed10.title#:#feed10.item[1].title#:#feed10.item[1].description.value#:#q10.recordcount#:#q10.title[1]#</cfoutput>

<!--- query + properties create --->
<cfset qc = QueryNew("title,content,publisheddate", "varchar,varchar,varchar")>
<cfset QueryAddRow(qc)>
<cfset QuerySetCell(qc, "title", "QFirst")>
<cfset QuerySetCell(qc, "content", "qdesc")>
<cfset QuerySetCell(qc, "publisheddate", "Mon, 06 Jan 2020 12:00:00 GMT")>
<cfset pcrss = {version="rss_2.0", title="QFeed", link="http://q", description="qd", encoding="UTF-8"}>
<cffeed action="create" query="#qc#" properties="#pcrss#" xmlvar="xq">
<cfoutput>|QCRSS:#Replace(xq, "#chr(13)##chr(10)#", "|", "ALL")#</cfoutput>
