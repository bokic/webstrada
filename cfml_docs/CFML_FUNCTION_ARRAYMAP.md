# Function Name: `ArrayMap`

## Description
Iterates over every entry of the array and calls the closure function to work on the element of the array. The returned value will be set at the same index in a new array and the new array will be returned

## Return Type
`array`

## Syntax
```cfml
arrayMap(array, function(item [,index, array]){} [, parallel] [, maxThreads])
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The input array

### Argument: `callback`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Closure or a function reference that will be called for each of the iteration.

### Argument: `parallel`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Lucee4.5+ true if the items can be executed in parallel

### Argument: `maxThreads`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `20`
- **Description**: Lucee4.5+ the maximum number of threads to use when parallel = true

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayReduce`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

