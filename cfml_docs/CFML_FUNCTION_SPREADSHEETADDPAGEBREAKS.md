# Function Name: `SpreadsheetAddPageBreaks`

## Description
A function to add page breaks for rows and columns to a Spreadsheet Object.

## Return Type
`void`

## Syntax
```cfml
spreadsheetAddPagebreaks(SpreadsheetObj, rowbreaks, colbreaks)
```

## Arguments

### Argument: `SpreadsheetObj`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Excel spreadsheet object to apply page break to.

### Argument: `rowbreaks`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Comma-delimited row numbers where the page breaks will be applied.

### Argument: `colbreaks`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Comma-delimited column numbers where the page breaks will be applied.

## Limitations and Other Info

- **Related Functions**: `SpreadsheetNew`, `SpreadsheetRead`
- **Coldfusion Support**: Minimum version: `11`.

