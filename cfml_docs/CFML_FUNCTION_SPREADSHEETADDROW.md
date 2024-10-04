# Function Name: `SpreadsheetAddRow`

## Description
Adds a row to an Excel spreadsheet object.

## Return Type
`void`

## Syntax
```cfml
spreadsheetAddRow(spreadsheetObj, data [, row] [, column] [, insert] [, datatype])
```

## Arguments

### Argument: `spreadsheetObj`
- **Type**: `variableName`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The spreadsheet

### Argument: `data`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A comma separated list of cell values.

### Argument: `row`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Row number at which to insert, if omitted appended.

### Argument: `column`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: Column number at which to insert data.

### Argument: `insert`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: When true appends data to spreadsheetObj, when false attempts to update rows.

### Argument: `datatype`
- **Type**: `array`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF11+ List of datatype expressions with values such as `STRING` `NUMERIC` or `DATE`. For example use `DATE:1;NUMERIC:2-2;STRING

## Limitations and Other Info

- **Related Functions**: `spreadsheetAddRows`
- **Coldfusion Support**: Minimum version: `9`.
- **Openbd Support**:

