# Function Name: `REFindNoCase`

## Description
Uses a regular expression (RE) to search a string for a pattern, starting from a specified position. The search is case-insensitive.

## Return Type
`any`

## Syntax
```cfml
reFindNoCase(reg_expression, String [, start] [, returnsubexpressions])
```

## Arguments

### Argument: `reg_expression`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string or a variable that contains one. String in which
 to search.

### Argument: `start`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: 

### Argument: `returnsubexpressions`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: True: if the regular expression is found, the first array
 element contains the length and position, respectively,
 of the first match.
 If the regular expression contains parentheses that
 group subexpressions, each subsequent array element
 contains the length and position, respectively, of
 the first occurrence of each group.
 If the regular expression is not found, the arrays each
 contain one element with the value 0.
 False: the function returns the position in the string
 where the match begins. Default.

### Argument: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `one`
- **Description**: CF2016+ * one: returns the first value that matches the regex.
 * all: returns all values that match the regex.

## Limitations and Other Info

- **Related Functions**: `Find`, `FindNoCase`, `REFind`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

