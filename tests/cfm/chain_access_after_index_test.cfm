<cfscript>
	// Chained member access after an index hop: s[key].a.b must walk hops,
	// never treat "a.b" as one literal key (regression: MangoBlog settings).
	s = { k1 = { k2 = { deep = "D" } }, K3 = "plain" };
	key = "k1";
	writeOutput("r1=" & s[key].k2.deep);            // bracket-var + 2 dots
	writeOutput("|r2=" & s["k1"].k2.deep);          // bracket-literal + 2 dots
	writeOutput("|r3=" & s[key]["k2"].deep);        // bracket, bracket, dot
	writeOutput("|r4=" & s[key].k2["deep"]);        // bracket, dot, bracket
	p = s[key].k2;
	writeOutput("|r5=" & p.deep);
	writeOutput("|r6=" & s.k1.k2.deep);             // pure dots (control)

	// writes through a chained target
	s[key].k2.deep = "W1";
	writeOutput("|w1=" & s.k1.k2.deep);
	s[key].k2.newKey = "W2";
	writeOutput("|w2=" & structKeyExists(s.k1.k2, "newkey"));
	s[key].k2.n = 1;
	s[key].k2.n += 41;
	writeOutput("|w3=" & s.k1.k2.n);

	// whitespace inside the glued span
	t = { a = { b = { c = "S" } } };
	key0 = "a";
	v = t[key0] . b . c;
	writeOutput("|ws=" & v);

	// 4-hop chain and array base
	deep = { a = { b = { c = { d = "B" } } } };
	k = "a";
	writeOutput("|h4=" & deep[k].b.c.d);
	arr = [ { inner = { val = "i0" } }, { inner = { val = "i1" } } ];
	writeOutput("|arr=" & arr[2].inner.val);
	arr[1].inner.val = "mut";
	writeoutput("|aw=" & arr[1].inner.val);

	// chained access used inline in call arguments and expressions
	writeOutput("|len=" & len(s[key].k2.deep));
	writeOutput("|cond=" & (s[key].k2.deep EQ "W1"));

	// function-scope variant (the Mango pattern)
	function grab(data, id) {
		var out = "x";
		out = arguments.data[arguments.id].k2.deep;
		return out;
	}
	writeOutput("|fn=" & grab(s, "k1"));
</cfscript>