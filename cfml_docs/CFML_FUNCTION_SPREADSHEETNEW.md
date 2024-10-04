# Function Name: `SpreadsheetNew`

## Description
 Creates a ColdFusion spreadsheet object, which represents a single sheet of an Excel document.

## Return Type
`any`

## Syntax
```cfml
spreadsheetNew([sheetname] [, xmlFormat])
```

## Arguments

### Argument: `sheetname`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: String value to be used as the sheet name.

### Argument: `xmlFormat`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Boolean value to indicate the use of Excels xlsx format.

## Limitations and Other Info

- **Related Functions**: `spreadsheetAddRow`, `spreadsheetRead`, `spreadsheetformatCell`, `spreadsheetWrite`
- **Coldfusion Support**: Minimum version: `9`.
- **Openbd Support**:

