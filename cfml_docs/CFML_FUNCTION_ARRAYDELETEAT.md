# Function Name: `ArrayDeleteAt`

## Description
Deletes the element at `index` from an array
The array will be resized, so that the deleted element doesn't leave a gap.

## Return Type
`boolean`

## Syntax
```cfml
arrayDeleteAt(array, index)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The array that the element will be deleted from.

### Argument: `index`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The numeric index of the element. Remember that ColdFusion arrays start at 1 not 0.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayDelete`, `arrayDeleteNoCase`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

