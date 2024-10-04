# Function Name: `SpreadsheetFormatCellRange`

## Description
Formats the cells within the given range.

## Return Type
`void`

## Syntax
```cfml
spreadsheetFormatCellRange (spreadsheetObj, format, startRow, startColumn, endRow, endColumn)
```

## Arguments

### Argument: `spreadsheetObj`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The Excel spreadsheet object for which you want to format the cells.

### Argument: `format`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A structure that contains format information.

### Argument: `startRow`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of the first row to format.

### Argument: `startColumn`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of the first column to format.

### Argument: `endRow`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of the last row to format.

### Argument: `endColumn`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of the last column to format.

## Limitations and Other Info

- **Related Functions**: `SpreadsheetFormatCell`, `SpreadsheetFormatColumns`, `SpreadsheetFormatRow`, `SpreadsheetFormatRows`
- **Coldfusion Support**: Minimum version: `9.0.1`.

