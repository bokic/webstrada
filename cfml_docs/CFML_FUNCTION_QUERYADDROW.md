# Function Name: `QueryAddRow`

## Description
 Adds a specified number of empty rows to a query.

## Return Type
`numeric`

## Syntax
```cfml
queryAddRow(query [, number/row(s)])
```

## Arguments

### Argument: `query`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `number/row(s)`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: As of CF10+ you can pass a Structure whose keys map to the query column names to insert a row of data; or an Array of those Structures to insert multiple rows at once.

## Limitations and Other Info

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `QueryAddColumn`, `QuerySetCell`, `QueryNew`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

