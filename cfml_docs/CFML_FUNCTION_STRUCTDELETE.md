# Function Name: `StructDelete`

## Description
Removes an element from a structure.

## Return Type
`boolean`

## Syntax
```cfml
structDelete(structure, key [, indicateNotExisting])
```

## Arguments

### Argument: `structure`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Structure or a variable that contains one. Contains element
 to remove

### Argument: `key`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Element to remove

### Argument: `indicateNotExisting`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: When true this function will return true only if the key that was deleted existed. When false (default) this function will return true if the key is successfully removed.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array. A component's `this` scope is also accepted because Adobe ColdFusion exposes it as a struct-compatible map.
- **Related Functions**: `structclear`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.
