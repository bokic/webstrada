# Function Name: `ListEach`

## Description
Iterates over every element of a List object and can call a UDF function, passed as the second argument.

## Return Type
`void`

## Syntax
```cfml
listEach(String str, UDFMethod function [, String delim, boolean includeEmptyFields]);
```

## Arguments

### Argument: `str`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An input list object.

### Argument: `function`
- **Type**: `function`
- **Required**: Required
- **Default Value**: *None*
- **Description**: UDF or closure object.

### Argument: `delim`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A list delimiter to be used. The default value is comma (,).

### Argument: `includeEmptyFields`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Boolean. Whether to allow empty fields. Default is false.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `arrayEach`, `structEach`, `queryEach`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

