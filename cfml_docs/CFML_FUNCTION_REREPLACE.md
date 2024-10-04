# Function Name: `REReplace`

## Description
Uses a regular expression (regex) to search a string for a string pattern and replace it with another. The search is case-sensitive.

## Return Type
`string`

## Syntax
```cfml
reReplace(string, regex, substring [, scope])
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string or a variable that contains one

### Argument: `regex`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Regular expression to replace.

### Argument: `substring`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string or a variable that contains one. Replaces substring with the regex match

### Argument: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `one`
- **Description**: * one: Replace the first occurrence of the regular
 expression. Default.
 * all: Replace all occurrences of the regular expression.

## Limitations and Other Info

- **Related Functions**: `rereplacenocase`, `refind`, `refindnocase`, `replace`, `rematch`, `rematchnocase`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

