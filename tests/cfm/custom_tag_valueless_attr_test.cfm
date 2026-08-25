<cfimport prefix="mytag" taglib="customtags">
T1:<mytag:valueless charset />
T2:<mytag:valueless title="foo" />
T3:<mytag:valueless a b />
T4:<mytag:valueless a b="x" />
T5:<mytag:valueless a="x" b />
T6:<mytag:valueless a="x" b c="z" />
M1:<cfmodule template="customtags/modvalueless.cfm" charset />
M2:<cfmodule template="customtags/modvalueless.cfm" charset="yes" />
M3:<cfmodule template="customtags/modvalueless.cfm" a b="x" />
