# CFML Operator Reference and Precedence

This reference document outlines all CFML operators, their aliases, functionality, and their **exact precedence hierarchy** verified live on the ColdFusion server.

---

## 1. Operator Precedence Hierarchy (Highest to Lowest)

| Rank | Operator Type | Operators / Aliases | Description | Verification Expression | ColdFusion Result |
| :---: | :--- | :--- | :--- | :--- | :--- |
| **1** | Parentheses | `()` | Explicit grouping | N/A | N/A |
| **2** | Exponentiation | `^` | Power operator | `2 ^ 3 * 2` | `16` (Deduces `^` > `*`) |
| **3** | Multiplicative | `*`, `/`, `\`, `%`, `MOD` | Multiplication, Division, Integer Division, Modulo | `2 + 3 * 4`<br>`5 + 5 MOD 3` | `14` (Deduces `*` > `+`) <br> `7` (Deduces `MOD` > `+`) |
| **4** | Additive | `+`, `-` | Addition, Subtraction | `'A' & 2 + 3` | `"A5"` (Deduces `+` > `&`) |
| **5** | Concatenation | `&` | String concatenation | `'a' & 'b' EQ 'ab'` | `YES` (Deduces `&` > `EQ`) |
| **6** | Comparison | `EQ`, `==`, `NEQ`, `!=`, `LT`, `<`, `LTE`, `<=`, `GT`, `>`, `GTE`, `>=`, `CONTAINS`, `DOES NOT CONTAIN`, `IS`, `IS NOT` | Value comparison (equality, inequality, inequality bounds, substring match) | `NOT 0 GT 3` | `YES` (Deduces `GT` > `NOT`) |
| **7** | Logical NOT | `NOT`, `!` | Logical negation | `not false and false` | `false` (Deduces `NOT` > `AND`) |
| **8** | Logical AND | `AND`, `&&` | Logical conjunction | `true or true and false` | `true` (Deduces `AND` > `OR`) |
| **9** | Logical OR | `OR`, `||` | Logical disjunction | `true xor false or true` | `NO` (Deduces `OR` > `XOR`) |
| **10** | Logical XOR | `XOR` | Logical exclusive disjunction | (Tested symmetrically) | Evaluates symmetrically with `EQV` |
| **11** | Logical EQV | `EQV` | Logical equivalence | `false imp false eqv false` | `YES` (Deduces `EQV` > `IMP`) |
| **12** | Logical IMP | `IMP` | Logical implication | `false imp false eqv false` | `YES` (Deduces `EQV` > `IMP`) |

> **Associativity (verified live on CF 2021):**
> * `^` is **left-associative**: `2^3^2` → `64` (= `(2^3)^2`), `3^2^2` → `81`, `2^2^3` → `64`, `5^2^1` → `25`. Use parentheses to force right-associativity: `2^(3^2)` → `512`.
> * Unary `-`/`+` bind **tighter** than `^`: `-2^2` → `4` (= `(-2)^2`), `-3^2` → `9`, `-2^3` → `-8`, `-2^1025` → `-1.#INF`. So `-x.y` still means `-(x.y)` (member access binds tighter than unary minus).

---

## 2. Exhaustive Operator Reference

### 2.1 Arithmetic Operators
*   `^` : Exponentiation (e.g. `2 ^ 3` evaluates to `8`).
*   `*` : Multiplication.
*   `/` : Floating-point division (e.g. `5 / 2` evaluates to `2.5`).
*   `\` : Integer division (e.g. `5 \ 2` evaluates to `2`).
*   `MOD` or `%` : Modulo / Remainder (e.g. `5 MOD 3` evaluates to `2`).
*   `+` : Addition.
*   `-` : Subtraction.

### 2.2 String Operators
*   `&` : String Concatenation. Automatically converts non-string operands (like numbers or booleans) to strings (e.g. `"A" & 5` evaluates to `"A5"`).

### 2.3 Comparison Operators
All comparison operators are case-insensitive and support both word-based aliases and symbolic aliases:
*   `EQ` or `==` or `equal` or `is` : Equal.
*   `NEQ` or `!=` or `not equal` or `is not` : Not Equal.
*   `LT` or `<` or `less than` : Less Than.
*   `LTE` or `<=` or `less than or equal to` or `le` : Less Than or Equal To.
*   `GT` or `>` or `greater than` : Greater Than.
*   `GTE` or `>=` or `greater than or equal to` or `ge` : Greater Than or Equal To.
*   `CONTAINS` : Checks if a string contains another substring (case-insensitive).
*   `DOES NOT CONTAIN` : Checks if a string does not contain a substring.

### 2.4 Logical Operators
*   `NOT` or `!` : Logical negation.
*   `AND` or `&&` : Logical conjunction.
*   `OR` or `||` : Logical disjunction.
*   `XOR` : Logical exclusive OR (returns true if one and only one operand is true).
*   `EQV` : Logical equivalence (returns true if both operands are true or both are false).
*   `IMP` : Logical implication (returns false only if the first operand is true and the second is false).

### 2.5 Assignment Operators
*   `=` : Simple assignment.
*   `+=`, `-=`, `*=`, `/=`, `%=`, `&=` : Compound assignment operators (evaluated at lowest precedence).

---

## 3. Precedence Verification Code Examples

These examples demonstrate verified evaluation differences for precedence ordering on the ColdFusion server:

### Exponentiation vs. Multiplication
```cfml
<cfset res = 2 ^ 3 * 2 />
<!--- 
Evaluates as (2^3) * 2 = 16 (since exponentiation has higher precedence).
If multiplication were higher, it would evaluate 2^(3*2) = 64.
--->
```

### Addition vs. Concatenation
```cfml
<cfset res = "A" & 2 + 3 />
<!--- 
Evaluates as "A" & (2 + 3) = "A5" (since addition has higher precedence).
If concatenation were higher, it would evaluate ("A" & 2) + 3 = "A2" + 3 and throw a cast exception.
--->
```

### Comparison vs. Logical NOT
```cfml
<cfset res = NOT 0 GT 3 />
<!--- 
Evaluates as NOT (0 > 3) = NOT false = true (since comparison has higher precedence).
If NOT were higher, it would evaluate (NOT 0) > 3 = true > 3 -> 1 > 3 = false.
--->
```
