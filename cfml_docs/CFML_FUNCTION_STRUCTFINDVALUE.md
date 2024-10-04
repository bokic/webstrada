# Function Name: `StructFindValue`

## Description
 Searches recursively through a substructure of nested arrays,
 structures, and other elements for structures with values that
 match the search key in the value parameter.

## Return Type
`array`

## Syntax
```cfml
structFindValue(top, value [, scope])
```

## Arguments

### Argument: `top`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: CFML object (a structure or an array) from which to
 start search. This attribute requires an object, not a
 name of an object.

### Argument: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String or a variable that contains one for which to search.
 The type must be a simple object. Arrays and structures
 are not supported.

### Argument: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: one: function returns one matching key (default)
 all: function returns all matching keys

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Related Functions**: `structFindKey`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

