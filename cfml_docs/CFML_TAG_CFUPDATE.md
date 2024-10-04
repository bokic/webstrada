# Tag Name: `cfupdate`

## Description
Updates records in a data source from data in a CFML form
 or form Scope.

## Syntax
```cfml
<cfupdate datasource="" tablename="">
```

## Attributes / Variants

### Attribute: `datasource`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the data source that contains the table

### Attribute: `tablename`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of table to update.
 For ORACLE drivers, must be uppercase.
 For Sybase driver: case-sensitive; must be in same case
 as used when the table was created

### Attribute: `tableowner`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For data sources that support table ownership (for example,
 SQL Server, Oracle, Sybase SQL Anywhere), the table owner.

### Attribute: `tablequalifier`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For data sources that support table qualifiers. The purpose
 of table qualifiers is as follows:
 SQL Server and Oracle: name of database that contains
 table
 Intersolv dBASE driver: directory of DBF files

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides username value specified in ODBC setup.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides password value specified in ODBC setup.

### Attribute: `formfields`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Comma-delimited list of form fields to update.

 If a form field is not matched by a column name in the
 database, CFML throws an error.

 The formFields list must include the database table primary
 key field, which must be present in the form. It can be
 hidden.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

