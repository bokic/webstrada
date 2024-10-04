# Function Name: `ArrayReduce`

## Description
Iterates over every entry of the array and calls the closure to work on the elements of the array. This function will reduce the array to a single value and will return the value.

## Return Type
`any`

## Syntax
```cfml
arrayReduce(array, function(result, item [,index, array]){} [, initialValue])
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: the input array

### Argument: `callback`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Closure or a function reference that will be called for each of the iteration.

### Argument: `initialValue`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Initial value which will be used for the reduce operation.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `ArrayMap`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

