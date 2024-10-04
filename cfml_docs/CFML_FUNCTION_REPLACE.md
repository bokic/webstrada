# Function Name: `Replace`

## Description
Replaces occurrences of substring1 in a string with obj, in a specified scope. The search is case-sensitive. Function returns original string with replacements made.

## Return Type
`string`

## Syntax
```cfml
replace(string, substring1, obj [, scope])
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String to search

### Argument: `substring1`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Substring to find within string

### Argument: `obj`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String to replace substring1 with. As of CF2016+ you can also pass a callback function in this argument `function(transform, position, original)`.

### Argument: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `one`
- **Description**: * one: replace the first occurrence
 * all: replace all occurrences

### Argument: `start`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: CF2021+ Position to start searching in the string.

## Limitations and Other Info

- **Related Functions**: `replaceNoCase`, `replaceList`, `reReplace`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

