# Function Name: `ArraySlice`

## Description
Returns a new array, from the start position up to the count of elements.

## Return Type
`array`

## Syntax
```cfml
arraySlice(array, offset, length)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the array that you want to slice

### Argument: `offset`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specifies the position from which to slice the array. Negative value indicates that the array is sliced, with sequence starting from array's end.

### Argument: `length`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum elements to slice

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `ArrayFind`, `ArrayFindNoCase`, `ArrayMap`, `ArrayReduce`, `ArrayResize`, `ArraySort`, `ArraySwap`, `ArrayToList`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

