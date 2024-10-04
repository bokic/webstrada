<cfoutput>
<cftry>E1:#REReplace("hello","","X")#<cfcatch>|E1err:#cfcatch.message#</cfcatch></cftry>
<cftry>E2:#REReplace("hello","l","X","nope")#<cfcatch>|E2err:#cfcatch.message#</cfcatch></cftry>
<cftry>E3:#REFind("l","hello",1,false,"nope")#<cfcatch>|E3err:#cfcatch.message#</cfcatch></cftry>
<cftry>E4:#REReplaceNoCase("hello","l","X","bad")#<cfcatch>|E4err:#cfcatch.message#</cfcatch></cftry>
<cftry>E5:#REFindNoCase("l","hello",1,false,"bad")#<cfcatch>|E5err:#cfcatch.message#</cfcatch></cftry>
</cfoutput>
