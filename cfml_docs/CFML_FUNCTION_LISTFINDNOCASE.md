# Function Name: `ListFindNoCase`

## Description
Determines the index of the first list element in which a specified value occurs. Returns 0 if not found. Case-insensitive.

## Return Type
`numeric`

## Syntax
```cfml
listFindNoCase(list, value [, delimiters, includeEmptyValues])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: List to search in.

### Argument: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String to search for.

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: String of character(s) that separate list elements.

### Argument: `includeEmptyFields`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: If includeEmptyFields is set to true, empty list elements will be counted when index of the first found list element is returned.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `listFind`, `arrayFindNoCase`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

