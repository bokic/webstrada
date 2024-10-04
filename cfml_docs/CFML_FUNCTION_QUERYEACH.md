# Function Name: `QueryEach`

## Description
Iterates over query rows and passes each row per iteration to a callback function

## Return Type
`void`

## Syntax
```cfml
queryEach(query, function(row [, currentRow] [, query] ){} [, parallel] [, maxThreads])
```

## Arguments

### Argument: `query`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: query to loop over

### Argument: `callback`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Closure or a function reference that will be called for each of the iteration.

### Argument: `parallel`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ Executes closures parallel

### Argument: `maxThreads`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `20`
- **Description**: Lucee4.5+ Maximum number of threads executed
If `parallel` argument is set to false it will be ignored

## Limitations and Other Info

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `QueryMap`, `QueryReduce`, `QueryFilter`
- **Lucee Support**: Notes: When called as a member function query.each() is chainable.
- **Coldfusion Support**: Minimum version: `2016`.
- **Boxlang Support**: Minimum version: `1.0.0`.

