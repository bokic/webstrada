# Function Name: `QueryKeyExists`

## Description
Determines whether the specified column (key) is present in a query.

## Return Type
`boolean`

## Syntax
```cfml
queryKeyExists(query, key)
```

## Arguments

### Argument: `query`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Query Object to test.

### Argument: `key`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Key to test

## Limitations and Other Info

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `QuerySort`, `QueryFilter`, `QueryEach`
- **Coldfusion Support**: Minimum version: `2016`.
- **Lucee Support**: Minimum version: `5.1`.
- **Boxlang Support**: Minimum version: `1.0.0`.

