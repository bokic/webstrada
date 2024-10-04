<!--- RandRange: deterministic output only (the random values differ per run).
    Equal bounds always return that value; a reversed range is swapped to
    [min,max]. Algorithm validation is per-thread (CF): an unsupported name
    throws only on the first call of a thread — see rand_algorithm_caching_test.cfm. --->
<cfoutput>#RandRange(100,100)#|#RandRange(7,7)#|#RandRange(-5,-5)#</cfoutput>
<cfset ok1 = 1>
<cfloop index="i" from="1" to="100">
	<cfset v = RandRange(1, 6)>
	<cfif v LT 1 OR v GT 6><cfset ok1 = 0></cfif>
</cfloop>
<cfset ok2 = 1>
<cfloop index="i" from="1" to="100">
	<cfset v = RandRange(6, 1)>
	<cfif v LT 1 OR v GT 6><cfset ok2 = 0></cfif>
</cfloop>
<cfset ok3 = 1>
<cfloop index="i" from="1" to="100">
	<cfset v = RandRange(-3, 3)>
	<cfif v LT -3 OR v GT 3><cfset ok3 = 0></cfif>
</cfloop>
<cfset ok4 = 1>
<cfloop index="i" from="1" to="100">
	<cfset v = RandRange(1, 2, "SHA1PRNG")>
	<cfif v LT 1 OR v GT 2><cfset ok4 = 0></cfif>
</cfloop>
<cfoutput>
#ok1#|#ok2#|#ok3#|#ok4#
</cfoutput>

