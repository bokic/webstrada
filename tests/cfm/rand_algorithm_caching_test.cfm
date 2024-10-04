<!--- Rand/RandRange/Randomize algorithm caching (per-thread, like CF): CF validates
    the algorithm only on the first Rand/RandRange call of a thread (its SecureRandom
    generator lives in a never-cleared ThreadLocal), while Randomize always validates
    (it reseeds via SecureRandom.getInstance every call). Every throw is caught below
    so the request never aborts; the output is status flags only (random values vary
    per run). Verified against CF 2025. --->
<cfset v = RandRange(1, 2, "SHA1PRNG")>
<cftry>
	<cfset x = RandRange(1, 2, "bogus")>
	<cfset r1 = "no-throw">
	<cfcatch type="any"><cfset r1 = "threw"></cfcatch>
</cftry>

<cfset z = Randomize(42)>
<cftry>
	<cfset x = RandRange(1, 2, "bogus")>
	<cfset r2 = "no-throw">
	<cfcatch type="any"><cfset r2 = "threw"></cfcatch>
</cftry>

<cftry>
	<cfset z2 = Randomize(42, "bogus")>
	<cfset r3 = "no-throw">
	<cfcatch type="any"><cfset r3 = "threw"></cfcatch>
</cftry>

<cftry>
	<cfset d = Rand("bogus")>
	<cfset r4 = "no-throw">
	<cfcatch type="any"><cfset r4 = "threw"></cfcatch>
</cftry>

<cftry>
	<cfset d = Rand()>
	<cfset x = RandRange(1, 2, "bogus")>
	<cfset r5 = "no-throw">
	<cfcatch type="any"><cfset r5 = "threw"></cfcatch>
</cftry>

<cftry>
	<cfset x = RandRange(1, 2, "NativePRNGBlocking")>
	<cfset r6 = "no-throw">
	<cfcatch type="any"><cfset r6 = "threw"></cfcatch>
</cftry>

<cftry>
	<cfset x = RandRange(1, 2, "Windows-PRNG")>
	<cfset r7 = "no-throw">
	<cfcatch type="any"><cfset r7 = "threw"></cfcatch>
</cftry>

<cftry>
	<cfset x = RandRange(1, 2, "sha1prng")>
	<cfset r8 = "no-throw">
	<cfcatch type="any"><cfset r8 = "threw"></cfcatch>
</cftry>

<cftry>
	<cfset z = Randomize(42, "SHA1PRNG")>
	<cfset r9 = "no-throw">
	<cfcatch type="any"><cfset r9 = "threw"></cfcatch>
</cftry>

<cftry>
	<cfset d = Rand("NativePRNG")>
	<cfset r10 = "no-throw">
	<cfcatch type="any"><cfset r10 = "threw"></cfcatch>
</cftry>

<!--- Randomize returns the next value of the freshly reseeded generator: a
    numeric in [0,1), deterministic for a given seed (CF: 0.832195441766 for
    seed 42). Verify the type/range/determinism without printing the value. --->
<cfset rr = Randomize(7)>
<cfset r11 = IsNumeric(rr) AND rr GT 0 AND rr LT 1>
<cfset r12 = Randomize(7) EQ rr>

<cfoutput>#r1#|#r2#|#r3#|#r4#|#r5#|#r6#|#r7#|#r8#|#r9#|#r10#|#r11#|#r12#</cfoutput>
