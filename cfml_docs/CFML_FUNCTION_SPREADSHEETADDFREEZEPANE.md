# Function Name: `SpreadsheetAddFreezePane`

## Description
Adds freeze pane (non-scrollable columns/rows) to spreadsheet

## Return Type
`void`

## Syntax
```cfml
spreadsheetAddFreezePane(spreadsheetObj, column, row [, endColumn] [, endRow])
```

## Arguments

### Argument: `spreadsheetObj`
- **Type**: `variableName`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Spreadsheet variable name

### Argument: `freezeColumn`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Amount of columns from left which should be freeze

### Argument: `freezeRow`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Amount of rows from top which should be freeze

### Argument: `hideColumn`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Amount of columns which should be hidden under/behind freeze columns (Scrolls left to first unhidden column).

### Argument: `hideRow`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Amount of rows which should be hidden under/behind freeze rows (Scrolls down to first unhidden row).

## Limitations and Other Info

- **Coldfusion Support**:
- **Openbd Support**:

