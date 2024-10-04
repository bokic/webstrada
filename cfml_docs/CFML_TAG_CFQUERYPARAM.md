# Tag Name: `cfqueryparam`

## Description
Verifies the data type of a query parameter and, for DBMSs that support bind variables, enables CFML to use bind variables in the SQL statement. Bind variable usage enhances performance when executing a cfquery statement multiple times.

 This tag is nested within a cfquery tag, embedded in a query SQL statement. If you specify optional parameters, this tag performs data validation.

NOTE: Due to security it's highly recommended to use this tag for any user input or non-static value used in a query to prevent code injections and the like.

## Syntax
```cfml
<cfqueryparam>
```

## Attributes / Variants

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Value that CFML passes to the right of the comparison operator in a where clause.

 If CFSQLType is a date or time option, ensure that the date value uses your DBMS-specific date format. Use the CreateODBCDateTime or DateFormat and TimeFormat functions to format the date value.

### Attribute: `cfsqltype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `CF_SQL_CHAR`
- **Description**: SQL type that parameter (any type) is bound to. As of CF11+ or Lucee4.5+ you can omit the `cf_sql_` prefix. 
See [CFSqlType Cheatsheet](https://cfdocs.org/cfsqltype-cheatsheet) for a mapping of CFSQL data types to DBMS data types.

### Attribute: `maxlength`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum length of parameter.

### Attribute: `scale`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Number of decimal places in parameter. Applies to `CF_SQL_NUMERIC` and `CF_SQL_DECIMAL`.

### Attribute: `null`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether parameter is passed as a `NULL` value.

 Yes: ignores the `value` attribute and passes `NULL`
 No: passes the `value` attribute

### Attribute: `list`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Yes: The value attribute value is a delimited list
 No: it is not

### Attribute: `separator`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Character that separates values in list, in value attribute.

## Limitations

- **Must be nested inside**: `cfquery`
- **Must not be nested inside**: *None*

