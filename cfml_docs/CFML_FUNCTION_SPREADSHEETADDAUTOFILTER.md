# Function Name: `SpreadsheetAddAutoFilter`

## Description
A function to add auto filters to the spreadsheet.

## Return Type
`void`

## Syntax
```cfml
spreadsheetAddAutoFilter(SpreadsheetObj, autofilter)
```

## Arguments

### Argument: `spreadsheetObj`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Excel spreadsheet object

### Argument: `autofilter`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The Excel filter that needs to be applied to the sheet. Note that the vale should not contain spaces or invalid characters.

## Limitations and Other Info

- **Related Functions**: `spreadsheetRead`, `spreadsheetNew`
- **Coldfusion Support**: Minimum version: `11`.

