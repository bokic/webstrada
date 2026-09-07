<cfscript>
// Post-increment: returns old value, then increments
i = 1;
i++;
WriteOutput(i & "|"); // 2

// Post-decrement: returns old value, then decrements
j = 5;
j--;
WriteOutput(j & "|"); // 4

// Post-increment inside expression (should yield old value)
k = 1;
WriteOutput(k++ & "|"); // 1
WriteOutput(k & "|");   // 2

// Pre-increment inside expression (should yield new value)
m = 1;
WriteOutput(++m & "|"); // 2
WriteOutput(m & "|");   // 2

// Pre-decrement inside expression
n = 10;
WriteOutput(--n & "|"); // 9
WriteOutput(n & "|");   // 9

// Post-decrement inside expression (should yield old value)
p = 10;
WriteOutput(p-- & "|"); // 10
WriteOutput(p & "|");   // 9

// Post-increment inside multiplication
a = 3;
result = a++ * 2;
WriteOutput(result & "|"); // 6 (3*2 old value)
WriteOutput(a & "|");      // 4

// Pre-increment inside multiplication
b = 3;
result2 = ++b * 2;
WriteOutput(result2 & "|"); // 8 (4*2 new value)
WriteOutput(b & "|");       // 4

// Post-decrement inside subtraction
c = 5;
result3 = c-- - 1;
WriteOutput(result3 & "|"); // 4 (5-1 old value)
WriteOutput(c & "|");       // 4

// Combined expression: post-inc + pre-inc
x = 3;
combined = x++ + ++x;
WriteOutput(combined & "|"); // 3 + 5 = 8
WriteOutput(x & "|");        // 5

// Increment/decrement on negative values
neg = -3;
neg++;
WriteOutput(neg & "|"); // -2

neg2 = -3;
++neg2;
WriteOutput(neg2 & "|"); // -2

neg3 = -3;
neg3--;
WriteOutput(neg3 & "|"); // -4

neg4 = -3;
--neg4;
WriteOutput(neg4 & "|"); // -4

// Compound assign operators (related - these should still work)
q = 1;
q += 2;
WriteOutput(q & "|"); // 3

r = 6;
r -= 1;
WriteOutput(r & "|"); // 5

s = 3;
s *= 3;
WriteOutput(s & "|"); // 9

t = 10;
t /= 4;
WriteOutput(t & "|"); // 2.5

u = "abc";
u &= "def";
WriteOutput(u);       // abcdef
</cfscript>
