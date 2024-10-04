# Function Name: `StructReduce`

## Description
Iterates over every entry of the struct and calls the closure to work on the key value pair of the struct. This function will reduce the struct to a single value and will return the value.

## Return Type
`any`

## Syntax
```cfml
structReduce(struct, function(result, key, value [,struct]){} [, initialVal])
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

### Argument: `initialVal`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Initial value which will be used for the reduce operation.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Related Functions**: `structEach`, `structSome`, `structMap`, `structFilter`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

