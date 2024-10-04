# Function Name: `StructInsert`

## Description
 Inserts a key-value pair into a structure.

## Return Type
`boolean`

## Syntax
```cfml
structInsert(structure, key, value [, allowoverwrite])
```

## Arguments

### Argument: `structure`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Structure to contain the new key-value pair.

### Argument: `key`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Key that contains the inserted value.

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Value to add.

### Argument: `allowoverwrite`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Whether to allow overwriting a key. Default: False.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

