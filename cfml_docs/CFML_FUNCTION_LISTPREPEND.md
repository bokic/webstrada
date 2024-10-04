# Function Name: `ListPrepend`

## Description
 Inserts an element at the beginning of a list.

## Return Type
`string`

## Syntax
```cfml
listPrepend(someList, value [, delimiters, includeEmptyFields])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string list.

### Argument: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An element or list of elements.

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: Characters that separate list elements.

### Argument: `includeEmptyFields`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF2018+ Set to true to insert blank values to the beginning of the list.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `listAppend`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

