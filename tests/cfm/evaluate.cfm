<cfset a = 5>
<cfset b = 2>
<cfset s = "hello">
<cfset t = true>
<cfset f = false>
<cfset expr = "1+1">
<cfset exprVar = "a + 1">
<cfset arr = [10, 20, 30]>
<cfset q = queryNew("id,name")>

<cfoutput>
BasicArithmetic:#Evaluate("1+1")#|
BasicAddVars:#Evaluate("a + b")#|
BasicConcat:#Evaluate("a & s")#|
BasicVarRef:#Evaluate("a")#|
InterpString:#Evaluate("#expr#")#|
InterpExpr:#Evaluate("#a# + #b#")#|
InterpVar:#Evaluate(exprVar)#|
MultiArgLeftToRight:#Evaluate("a", "b")#|
MultiArgRightUsesLeft:#Evaluate("a", "b + a")#|
MultiArgConcat:#Evaluate("a", "s")#|
DeInside:#Evaluate("de(a)")#|
DeAsArg:#Evaluate(de("a"))#|
SingleQuotedLiteral:#Evaluate("'a'")#|
SingleQuotedEscaped:#Evaluate("'it''s'")#|
DoubleQuotedEscaped:#Evaluate("""q""")#|
BooleanEq:#Evaluate("a EQ 5")#|
BooleanNeq:#Evaluate("a EQ 6")#|
BooleanAndBoolVars:#Evaluate("a EQ 5 AND t")#|
BooleanAndFalseLeft:#Evaluate("a EQ 6 AND t")#|
BooleanOr:#Evaluate("a EQ 5 OR t")#|
BooleanNot:#Evaluate("NOT t")#|
BooleanNested:#Evaluate("a EQ 5 AND (t OR f)")#|
BooleanGt:#Evaluate("a GT 3")#|
Mul:#Evaluate("2 * 3")#|
Div:#Evaluate("10 / 4")#|
Mod:#Evaluate("3 MOD 2")#|
Pow:#Evaluate("2^3")#|
Parens:#Evaluate("(a + b) * 2")#|
ArrayIndex:#Evaluate("arr[2]")#|
QueryEmptyCol:#Evaluate("q.id")#|
EmptyString:#Evaluate("")#|
NestedEvaluate:#Evaluate("evaluate('1+1')")#|
ComputedArgArithmetic:#Evaluate(5 + 3)#|
ComputedArgVars:#Evaluate(a + b)#|
ComputedArgConcat:#Evaluate("a" & "+" & "b")#|
</cfoutput>

<cfscript>
c = evaluate("1+1");
d = evaluate("a * b");
e = evaluate("a", "b");
f2 = evaluate("'x'");
</cfscript>
<cfoutput>
ScriptArithmetic:#c#|
ScriptMul:#d#|
ScriptMulti:#e#|
ScriptStringLiteral:#f2#|
</cfoutput>
