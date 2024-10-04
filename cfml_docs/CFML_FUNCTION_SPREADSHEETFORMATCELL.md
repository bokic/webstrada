# Function Name: `SpreadsheetFormatCell`

## Description
Formats the contents of a single cell of an Excel spreadsheet object.

## Return Type
`void`

## Syntax
```cfml
spreadsheetFormatCell(spreadsheetObj, format, row, column)
```

## Arguments

### Argument: `spreadsheetObj`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The spreadsheet object for which formatting will be applied

### Argument: `format`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A structure containing the formatting information

### Argument: `row`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The row the cell to format is in

### Argument: `column`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The column the cell to format is in

## Limitations and Other Info

- **Related Functions**: `SpreadsheetFormatColumn`, `SpreadsheetFormatColumns`, `SpreadsheetFormatRow`, `SpreadsheetFormatRows`
- **Coldfusion Support**: Minimum version: `9`.
- **Openbd Support**:

