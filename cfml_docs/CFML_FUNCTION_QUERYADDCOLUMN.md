# Function Name: `QueryAddColumn`

## Description
 Adds a column to a query and populates its rows with the
 contents of a one-dimensional array. Pads query columns,
 if necessary, to ensure that all columns have the same number
 of rows.

## Return Type
`numeric`

## Syntax
```cfml
queryAddColumn(query, column_name [, datatype], array_name)
```

## Arguments

### Argument: `query`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `column_name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `datatype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Column data type.

### Argument: `array_name`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

## Limitations and Other Info

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `QueryNew`, `QueryAddRow`, `QuerySetCell`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

