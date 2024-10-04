# Function Name: `SpreadsheetFormatRows`

## Description
Formats the contents of a multiple rows of an Excel spreadsheet object.

## Return Type
`void`

## Syntax
```cfml
spreadsheetFormatRows(spreadsheetObj, format, rows)
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

### Argument: `rows`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string containing comma-separated row numbers or row ranges

## Limitations and Other Info

- **Related Functions**: `SpreadsheetFormatRow`, `SpreadsheetFormatCell`
- **Coldfusion Support**: Minimum version: `9`.
- **Openbd Support**:

