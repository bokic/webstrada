<cfoutput>
A:#ReplaceList("This is a test", "is,test", "was,exam")#
B:#ReplaceList("a,b,c,d", "a,d", "X,Y")#
C:#ReplaceList("a&b&c", "a,c", "x,z", "&", ",")#
D:#ReplaceList("a;b;c", "a;b", "1;2", ";", ";")#
E:#ReplaceList("xyzabc", "a,b,c", "1,2,3", "")#
F:#ReplaceList("a b c", "a,b,c", "1,2,3", " ", " ")#
G:#ReplaceList("abc", "a,b,c", "X")#
H:#ReplaceList("abc", "a,b,c", "X,,Z")#
I:#ReplaceList("abc", "a,b,c", "X,Y,Z,W")#
J:#ReplaceList("a b c", "a,b,c", "1,2,3", "true")#
K:#ReplaceList("a b c", "a,b,c", "1,2,3", ",", "true")#
L:#ReplaceList("AAA", "a", "x")#
M:#ReplaceList("a1a2a", "a", "Q")#
N:#ReplaceList("banana", "ana,an", "XY,ZZ")#
O:#ReplaceList("abc", "b", "12")#
P:#ReplaceList("xx", ",a,b", "1,2")#
Q:#ReplaceList("", "a", "1")#
R:#ReplaceList("abc", "", "1")#
S:#ReplaceList("abc", "a,b", ",,1")#
T:#ReplaceList("one two three", "one,three", "1,3")#
U:#ReplaceList("abab", "ab", "x")#
V:#ReplaceList("abab", "b", "")#
W:#ReplaceList("abab", "a,b", "X,,Y")#
X:#ReplaceList("a|b|c", "a,b,c", "1,2,3", "|", "|")#
Y:#ReplaceList("a.b.c", "a,b,c", "1,2,3", ".", ".")#
Z:#ReplaceList("hello world", "o", "0")#
<cfscript>
s1 = ReplaceList("This is a test", "is,test", "was,exam");
s2 = ReplaceList("banana", "ana,an", "XY,ZZ");
s3 = ReplaceList("abc", "a,b,c", "X,,Z");
s4 = ReplaceList("xyzabc", "a,b,c", "1,2,3", "");
s5 = ReplaceList("a b c", "a,b,c", "1,2,3", "true");
s6 = ReplaceList("a;b;c", "a;b", "1;2", ";", ";");
s7 = ReplaceList("abab", "a,b", "X,,Y");
writeOutput("S1:#s1#|S2:#s2#|S3:#s3#|S4:#s4#|S5:#s5#|S6:#s6#|S7:#s7#");
</cfscript>
</cfoutput>
