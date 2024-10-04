# Function Name: `StructFindKey`

## Description
 Searches recursively through a substructure of nested arrays,
 structures, and other elements, for structures whose keys
 match the search key in the value parameter.

## Return Type
`array`

## Syntax
```cfml
structFindKey(top, value, scope)
```

## Arguments

### Argument: `top`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: CFML object (structure or array) from which to start
 search. This attribute requires an object, not a name of
 an object.

### Argument: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String or a variable that contains one for which to search.

### Argument: `scope`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: * one: returns one matching key. Default.
 * all: returns all matching keys

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Related Functions**: `structFindValue`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

