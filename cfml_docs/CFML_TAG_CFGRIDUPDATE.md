# Tag Name: `cfgridupdate`

## Description
Used within a cfgrid tag. Updates data sources directly from
 edited grid data. This tag provides a direct interface with
 your data source.

 This tag applies delete row actions first, then insert row
 actions, then update row actions. If it encounters an error,
 it stops processing rows.

## Syntax
```cfml
<cfgridupdate grid="" datasource="" tablename="">
```

## Attributes / Variants

### Attribute: `grid`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of cfgrid form element that is the source for the
 update action.

### Attribute: `datasource`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of data source for the update action.

### Attribute: `tablename`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Table in which to insert form fields.

 ORACLE drivers: must be uppercase.
 Sybase driver: case-sensitive. Must be the same case used
 when table was created

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides username specified in ODBC setup.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides password specified in ODBC setup.

### Attribute: `tableowner`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Table owner, if supported.

### Attribute: `tablequalifier`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For data sources that support table qualifiers, use this
 field to specify qualifier for table. The purpose of table
 qualifiers varies among drivers. For SQL Server and
 Oracle, qualifier refers to name of database that contains
 table. For Intersolv dBASE driver, qualifier refers to
 directory where DBF files are located.

### Attribute: `keyonly`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies to the update action:
 Yes: the WHERE criteria are limited to the key values
 No: the WHERE criteria include key values and the original
 values of changed fields

## Limitations

- **Must be nested inside**: `cfgrid`
- **Must not be nested inside**: *None*

