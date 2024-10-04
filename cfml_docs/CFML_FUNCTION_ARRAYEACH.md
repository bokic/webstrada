# Function Name: `ArrayEach`

## Description
Used to iterate over an array and run the function closure for each item in the array.

## Return Type
`void`

## Syntax
```cfml
arrayEach(array, function(item, [index, [array]]){} [, parallel] [, maxThreads])
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `callback`
- **Type**: `function`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Closure or a function reference that will be called for each of the iteration.

### Argument: `parallel`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Lucee4.5+ CF2021+ Specifies whether the items can be executed in parallel

### Argument: `maxThreads`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `20`
- **Description**: Lucee4.5+ CF2021+ The maximum number of threads to use when parallel = true

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayMap`, `arrayReduce`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

