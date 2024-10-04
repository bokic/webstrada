# Function Name: `QueryGetRow`

## Description
Returns a struct having all the columns as keys and their corresponding values.

## Return Type
`struct`

## Syntax
```cfml
queryGetRow( query, rowNumber );
```

## Arguments

### Argument: `query`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: query object do get data from.

### Argument: `rowNumber`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: position of the row to be returned.

## Limitations and Other Info

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `queryColumnData`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Transpiled to `queryRowData` in BoxLang.

