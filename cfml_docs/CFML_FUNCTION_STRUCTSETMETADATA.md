# Function Name: `StructSetMetadata`

## Description
Sets metadata for a key in a struct. When you want to SerializeJSON, the key and the value will be display as you defined in the metadata.

## Return Type
`void`

## Syntax
```cfml
structSetMetadata(inputStruct, metaStruct)
```

## Arguments

### Argument: `inputStruct`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The struct in which you want to add the metadata.

### Argument: `metaStruct`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The metadata struct you want to add.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Related Functions**: `arraySetMetadata`, `arrayGetMetadata`, `structGetMetadata`
- **Coldfusion Support**: Minimum version: `2016.0.2`.

