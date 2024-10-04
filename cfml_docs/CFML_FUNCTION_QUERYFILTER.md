# Function Name: `QueryFilter`

## Description
Filters query rows specified in filter criteria

## Return Type
`query`

## Syntax
```cfml
queryFilter(query, function(row [, currentRow] [, query] ){} [, parallel] [, maxThreads])
```

## Arguments

### Argument: `query`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The query to filter

### Argument: `callback`
- **Type**: `boolean`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Closure or a function reference that will be called for each of the iteration. Returns true if the row should be included in the filtered query.

### Argument: `parallel`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Lucee4.5+ CF2021+ true if the items can be executed in parallel

### Argument: `maxThreads`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `20`
- **Description**: Lucee4.5+ CF2021+ the maximum number of threads to use when parallel = true

## Limitations and Other Info

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `querySome`, `queryReduce`, `queryMap`, `queryEach`
- **Lucee Support**:
- **Coldfusion Support**: Minimum version: `2016`. Notes: queryFilter mutated the original query in CF2016 and 2018 (bug report: https://tracker.adobe.com/#/view/CF-4203366)
- **Boxlang Support**: Minimum version: `1.0.0`.

