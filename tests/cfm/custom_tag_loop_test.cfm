<cfimport prefix="mytag" taglib="customtags">
<mytag:looper times="2">
[ITER:<cfoutput>#loopCount#</cfoutput>]
</mytag:looper>
<mytag:looper times="4">
[ITER2:<cfoutput>#loopCount#</cfoutput>]
</mytag:looper>
