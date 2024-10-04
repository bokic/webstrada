# Function Name: `ArraySetMetadata`

## Description
Sets metadata for items of an array. Useful when using serializeJSON with ambiguous data.

## Return Type
`void`

## Syntax
```cfml
arraySetMetadata(array, metadata)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The array for which to set the metadata.

### Argument: `metadata`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The metadata struct to set.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayGetMetadata`, `structGetMetadata`, `structSetMetadata`
- **Coldfusion Support**: Minimum version: `2016.0.2`.

