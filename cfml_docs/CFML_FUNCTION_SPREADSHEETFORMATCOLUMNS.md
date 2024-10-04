# Function Name: `SpreadsheetFormatColumns`

## Description
Formats the contents of a multiple columns of an Excel spreadsheet object.

## Return Type
`void`

## Syntax
```cfml
spreadsheetFormatColumns(spreadsheetObj, format, columns)
```

## Arguments

### Argument: `spreadsheetObj`
- **Type**: `variableName`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The spreadsheet object for which formatting will be applied

### Argument: `format`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A structure containing the formatting information

### Argument: `columns`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string containing comma-separated column numbers or column ranges

## Limitations and Other Info

- **Related Functions**: `SpreadsheetFormatColumn`, `SpreadsheetFormatCell`
- **Coldfusion Support**: Minimum version: `9`.
- **Openbd Support**:

