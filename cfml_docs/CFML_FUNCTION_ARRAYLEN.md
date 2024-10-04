# Function Name: `ArrayLen`

## Description
Determines the largest index in an array.
For dense arrays, this corresponds to the number of elements in the array.
 CF MX: this function can be used on child XML objects.

## Return Type
`numeric`

## Syntax
```cfml
arrayLen(array)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An array from which to get the final index.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayAvg`
- **Coldfusion Support**: Minimum version: `3`. Notes: The member function cannot be called on literals
- **Lucee Support**:
- **Openbd Support**:

