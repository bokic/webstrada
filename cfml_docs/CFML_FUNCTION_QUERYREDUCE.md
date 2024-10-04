# Function Name: `QueryReduce`

## Description
Reduces query columns just like in array or collection

## Return Type
`any`

## Syntax
```cfml
queryReduce(query, function(result, row [, currentRow] [, query]){} [, initialVal])
```

## Arguments

### Argument: `query`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: query to process entries from

### Argument: `callback`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Closure or a function reference that will be called for each of the iteration.

### Argument: `initialValue`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: initial value passed as part of the first closure call

## Limitations and Other Info

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `querymap`, `queryfilter`
- **Lucee Support**:
- **Coldfusion Support**: Minimum version: `2016`.
- **Boxlang Support**: Minimum version: `1.0.0`.

