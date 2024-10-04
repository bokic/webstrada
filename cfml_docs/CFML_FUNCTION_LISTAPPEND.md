# Function Name: `ListAppend`

## Description
Concatenates a list or element to a list and returns the concatenated list.

## Return Type
`string`

## Syntax
```cfml
listAppend(list, value [, delimiters, includeEmptyFields])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A list or variable with the list.

### Argument: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An element or a list of elements.

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: A string or variable with a character that separates list elements.

### Argument: `includeEmptyFields`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF2018+ Set to true to append blank values to the list.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `listPrepend`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

