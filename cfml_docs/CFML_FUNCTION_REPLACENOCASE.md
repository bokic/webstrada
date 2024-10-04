# Function Name: `ReplaceNoCase`

## Description
Replaces occurrences of substring1 with callback, in the specified scope. The search is case-insensitive.

## Return Type
`string`

## Syntax
```cfml
replaceNoCase(string, substring1, callback [, scope])
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string (or variable that contains one) within which to replace substring

### Argument: `substring1`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string (or variable that contains one) to replace, if found.

### Argument: `callback`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String to replace substring1 with. As of CF2018+ you can also pass a callback function in this argument `function(transform, position, original)`.

### Argument: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: * one: Replace the first occurrence (default)
 * all: Replace all occurrences

### Argument: `start`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: CF2021+ Position to start searching in the string.

## Limitations and Other Info

- **Related Functions**: `replace`, `replaceList`, `reReplace`, `reReplaceNoCase`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

