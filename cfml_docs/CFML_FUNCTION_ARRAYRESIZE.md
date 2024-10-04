# Function Name: `ArrayResize`

## Description
Resets an array to a specified minimum number of elements.
 This can improve performance, if used to size an array to its
 expected maximum. For more than 500 elements, use arrayResize
 immediately after using the ArrayNew tag.

## Return Type
`boolean`

## Syntax
```cfml
arrayResize(array, size)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `size`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

