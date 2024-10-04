# Function Name: `ArrayFilter`

## Description
Used to filter an array to items for which the closure function returns true.

## Return Type
`array`

## Syntax
```cfml
arrayFilter(array, function(item [,index, array]){} [, parallel] [, maxThreads])
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `callback`
- **Type**: `boolean`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Closure or a function reference that will be called for each of the iteration. Returns true if the array element should be included in the filtered array. Support for passing the original array to the closure function added in CF11 Update 5.

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
- **Related Functions**: `arrayEach`, `arraySome`, `ArrayMap`, `arrayReduce`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

