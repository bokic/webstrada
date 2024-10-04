# Function Name: `ArraySwap`

## Description
Swaps array values of an array at specified positions. This function is more efficient than multiple cfset tags

## Return Type
`boolean`

## Syntax
```cfml
arraySwap(array, position1, position2)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The array for which positions will be swapped

### Argument: `position1`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Position of 1st element to swap.

### Argument: `position2`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Position of 2nd element to swap.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arraySet`, `arrayInsertAt`, `arrayReverse`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

