# Function Name: `ArraySet`

## Description
In a one-dimensional array, sets the elements in a specified
 index range to a value. Useful for initializing an array after
 a call to arrayNew.

## Return Type
`boolean`

## Syntax
```cfml
arraySet(array, start, end, value)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `start`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `end`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayInsertAt`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

