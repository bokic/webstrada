# Function Name: `StructFilter`

## Description
Used to filter the key-value pairs in a structure.

## Return Type
`struct`

## Syntax
```cfml
structFilter(struct,function(key, value [,struct]){} [, parallel] [, maxThreads])
```

## Arguments

### Argument: `struct`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the structure to filter

### Argument: `callback`
- **Type**: `boolean`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Closure or a function reference that will be called for each of the iteration. Returns true if the key-value pair in the structure should be included in the filtered struct. Support for passing the original struct to the closure function added in CF11 Update 5.

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
- **Related Functions**: `structEach`, `structSome`, `structMap`, `structReduce`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

