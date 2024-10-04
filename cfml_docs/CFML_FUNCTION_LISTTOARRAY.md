# Function Name: `ListToArray`

## Description
 Copies the elements of a list to an array.

## Return Type
`array`

## Syntax
```cfml
listToArray(list [, delimiters] [, includeEmptyFields] [, multiCharacterDelimiter])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A list or variable name

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: 

### Argument: `includeEmptyFields`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: A Boolean value specifying whether to create empty array entries if there are two delimiters in a row.

### Argument: `multiCharacterDelimiter`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: A Boolean value specifying whether the delimiters parameter specifies a multi-character delimiter.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `ArrayToList`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

