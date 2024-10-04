# Function Name: `StructMap`

## Description
Iterates over every entry of the Struct and calls the closure function to work on the key value pair of the struct. The returned value will be set for the same key in a new struct and the new struct will be returned.

## Return Type
`struct`

## Syntax
```cfml
structMap(struct, function(key, value [,struct]){} [, parallel] [, maxThreads])
```

## Arguments

### Argument: `struct`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The input struct.

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

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Related Functions**: `structEach`, `structSome`, `structReduce`, `structFilter`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**: Minimum version: `4.5`.
- **Boxlang Support**: Minimum version: `1.0.0`.

