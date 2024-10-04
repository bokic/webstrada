# Function Name: `QuerySetCell`

## Description
 Sets a cell to a value. If no row number is specified,
 the cell on the last row is set.

## Return Type
`boolean`

## Syntax
```cfml
querySetCell(query, column, value [, row])
```

## Arguments

### Argument: `query`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `column`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `row`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: 

## Limitations and Other Info

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `QueryAddColumn`, `QueryAddRow`, `QueryNew`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

