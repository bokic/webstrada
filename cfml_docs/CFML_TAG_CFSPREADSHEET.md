# Tag Name: `cfspreadsheet`

## Description
Read and writes Microsoft Excel spreadsheet files.

## Syntax
```cfml
<cfspreadsheet>
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: read - Reads the contents of an XLS format file.
update - Adds a new sheet to an existing XLS file. You cannot use the update action to change a sheet in an existing file.
write - Writes a new XLS format file or overwrites an existing file.

### Attribute: `autosize`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: CF11+ Toggles automatically adjusting the width of columns to accommodate their contents.

### Attribute: `columnnames`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Comma-separated column names.

### Attribute: `columns`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Column number or range of columns. Specify a single number, a hyphen-separated column range, a comma-separated list, or any combination of these; for example: 1,3-6,9.

### Attribute: `excludeHeaderRow`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF9.0.1+ If set to true, excludes the headerRow from being included in the query results of a spreadsheet read..

### Attribute: `filename`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The pathname of the file that is written.

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Format of the data represented by the name variable.

### Attribute: `headerrow`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Row number that contains column names.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: `action="read"` The variable in which to store the spreadsheet file data. You must specify name or query. If format="csv" then name will contain csv variable.
if format="html" then name will contain HTML content.
`action="write|update"` A variable containing CSV-format data or an ColdFusion spreadsheet object containing the data to write. You must specify name or query.

### Attribute: `overwrite`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value specifying whether to overwrite an existing file.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Set a password for modifying the sheet.

### Attribute: `query`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: `action="read"` The query in which to store the converted spreadsheet file. You must specify format, name, or query.
`action="write|update"` A query variable containing the data to write. You must specify name or query.

### Attribute: `rows`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The range of rows to read. Specify a single number, a hyphen-separated row range, a comma-separated list, or any combination of these. For example: 1,3-6,9.

### Attribute: `sheet`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Number of the sheet.

### Attribute: `sheetname`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the sheet.

### Attribute: `src`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The pathname of the file to read.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

