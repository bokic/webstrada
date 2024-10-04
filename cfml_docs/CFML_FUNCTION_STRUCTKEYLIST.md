# Function Name: `StructKeyList`

## Description
 Extracts keys from a CFML structure.

## Return Type
`string`

## Syntax
```cfml
structKeyList(structure [, delimiter])
```

## Arguments

### Argument: `structure`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Structure from which to extract a list of keys

### Argument: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: Character that separates keys in list. Default: comma.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Related Functions**: `structKeyArray`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

