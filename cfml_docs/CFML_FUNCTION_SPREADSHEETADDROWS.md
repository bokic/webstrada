# Function Name: `SpreadsheetAddRows`

## Description
Adds multiple rows from a query or array to an Excel spreadsheet object.

## Return Type
`void`

## Syntax
```cfml
spreadsheetAddRows(spreadsheetObj, data,[ row, column , insert, datatype, includeColumnNames])
```

## Arguments

### Argument: `spreadsheetObj`
- **Type**: `variableName`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The spreadsheet object variable

### Argument: `data`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A query or array

### Argument: `row`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The row number in the spreadsheet at which to insert the data. If omitted rows are appended.

### Argument: `column`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The column number to start, all columns to the left will be empty.

### Argument: `insert`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: When true appends the row `data` to the `spreadsheetObj`. When `false` attempts to update the spreadsheet object rows.

### Argument: `datatype`
- **Type**: `array`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF11+ An array of datatype expressions with values `STRING` `NUMERIC` or `DATE`. For example use `DATE:1;NUMERIC:2-2;STRING

### Argument: `includeColumnNames`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF2016+ When `true` writes column names as headers in the spreadsheet.

## Limitations and Other Info

- **Related Functions**: `spreadsheetAddRow`
- **Coldfusion Support**: Minimum version: `9`.
- **Openbd Support**:

