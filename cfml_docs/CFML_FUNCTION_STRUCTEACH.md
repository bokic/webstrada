# Function Name: `StructEach`

## Description
Used to loop over elements in a structure by accessing key-value pairs.

## Return Type
`void`

## Syntax
```cfml
structEach(struct,function(key, value [, struct]){} [, parallel] [, maxThreads])
```

## Arguments

### Argument: `struct`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Structure or a variable that contains one.

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
- **Related Functions**: `structSome`, `structMap`, `structReduce`, `structFilter`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

