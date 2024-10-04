# Function Name: `ListFind`

## Description
Determines the index of the first list element in which a specified value occurs. Returns 0 if not found. Case-sensitive

## Return Type
`numeric`

## Syntax
```cfml
listFind(list, value [, delimiters, includeEmptyValues])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: 

### Argument: `includeEmptyValues`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: 

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `listFindNoCase`, `arrayFind`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

