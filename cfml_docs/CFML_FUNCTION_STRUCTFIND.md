# Function Name: `StructFind`

## Description
Determines the value associated with a key in a structure.

## Return Type
`any`

## Syntax
```cfml
structFind(structure, key [, defaultValue ])
```

## Arguments

### Argument: `structure`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Structure that contains the value to return

### Argument: `key`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Key whose value to return

### Argument: `defaultValue`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ Default value which will be returned if the key does not exist or if null was found. Currently only supported by Lucee. See https://docs.lucee.org/reference/functions/structfind.html#argument-defaultValue

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Related Functions**: `structFindKey`, `structFindValue`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

