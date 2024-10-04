# Function Name: `REReplaceNoCase`

## Description
Uses a regular expression to search a string for a string
 pattern and replace it with another. The search is
 case-insensitive.

## Return Type
`string`

## Syntax
```cfml
reReplaceNoCase(String, reg_expression, substring[, scope])
```

## Arguments

### Argument: `String`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string or a variable that contains one

### Argument: `reg_expression`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Regular expression to replace.

### Argument: `substring`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string or a variable that contains one. Replaces
 reg_expression

### Argument: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: * one: Replace the first occurrence of the regular
 expression. Default.
 * all: Replace all occurrences of the regular expression.

## Limitations and Other Info

- **Related Functions**: `Replace`, `ReplaceList`, `ReplaceNoCase`, `REReplace`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

