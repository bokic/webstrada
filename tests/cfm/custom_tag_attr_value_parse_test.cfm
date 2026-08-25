<cfimport prefix="mytag" taglib="customtags">
<cfset foo = structNew()>
<cfset foo.bar = "FOOBAR">
A2:<mytag:atr a=#foo.bar# b="x" />
A3:<mytag:atr a="x" b="y" />
A5:<mytag:atr a="x" b />
B2:<mytag:atr a=1 b="x" />
B3:<mytag:atr a=true b="x" />
