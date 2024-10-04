# Function Name: `QueryNew`

## Description
Creates a new query object. The query can be populated with data using functions queryAddRow, querySetCell, or by passing it in to the rowData argument.

## Return Type
`query`

## Syntax
```cfml
queryNew(columnList [, columnTypeList [, rowData]])
```

## Arguments

### Argument: `columnList`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string or a variable that contains one. Delimited list
 of column names, or an empty string.

### Argument: `columnTypeList`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF7+ Comma-delimited list specifying column data types.

### Argument: `rowData`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ Data to populate the query. Can be a struct (with keys matching column names), an array of structs, or an array of arrays (in same order as columnList)

## Limitations and Other Info

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `queryaddrow`, `querysetcell`, `queryaddcolumn`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

