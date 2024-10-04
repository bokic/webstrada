# Function Name: `QueryMap`

## Description
Maps each query row using a function to manipulate the rows fields

## Return Type
`query`

## Syntax
```cfml
queryMap(query, function(row [, currentRow] [, query] ){} [, parallel] [, maxThreads])
```

## Arguments

### Argument: `query`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: query to process entries from

### Argument: `callback`
- **Type**: `boolean`
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

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `queryFilter`, `queryReduce`, `querySome`, `queryEach`
- **Lucee Support**:
- **Coldfusion Support**: Minimum version: `2016`.
- **Boxlang Support**: Minimum version: `1.0.0`.

