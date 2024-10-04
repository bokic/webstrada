# Function Name: `ListGetAt`

## Description
Gets a list element at a specified position.

## Return Type
`string`

## Syntax
```cfml
listGetAt(list, position [, delimiters [, includeEmptyValues]])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `position`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: 

### Argument: `includeEmptyValues`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: 

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `listDeleteAt`, `listInsertAt`, `listSetAt`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

