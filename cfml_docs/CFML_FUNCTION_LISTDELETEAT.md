# Function Name: `ListDeleteAt`

## Description
Deletes an element from a list.
 Returns a copy of the list, without the specified element.

## Return Type
`string`

## Syntax
```cfml
listDeleteAt(list, position [, delimiters])
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

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: 

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `listGetAt`, `listInsertAt`, `listSetAt`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

