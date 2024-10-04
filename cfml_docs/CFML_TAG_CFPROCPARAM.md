# Tag Name: `cfprocparam`

## Description
Defines stored procedure parameters.
This tag is nested within a `cfstoredproc` tag.
This tag does not have a body.

## Syntax
```cfml
<cfprocparam cfsqltype="CF_SQL_BIGINT">
```

## Attributes / Variants

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `in`
- **Description**: This attribute indicates whether the passed variable is an input, output, or input/output.
`in`: The parameter is used to send data to the database system only. Passes the parameter by value.
`out`: The parameter is used to receive data from the database system only. Passes the parameter as a bound variable.
`inout`: The parameter is used to send and receive data. Passes the parameter as a bound variable.

### Attribute: `variable`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the variable that references the value of the output parameter after the stored procedure is called.
Only valid when `type` attribute is `OUT` or `INOUT`.

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The actual value that is passed to the stored procedure.

### Attribute: `cfsqltype`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: SQL type to which the parameter (any type) is bound.
 CFML supports the following values, where the last element of the name corresponds to the SQL data type. Different database systems might support different subsets of this list. See your DBMS documentation for information on supported parameter types.
Refer to https://helpx.adobe.com/coldfusion/cfml-reference/coldfusion-tags/tags-p-q/cfqueryparam.html for how the types are mapped.

### Attribute: `maxlength`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Maximum length of a string or character IN or INOUT value attribute. A maxLength of `0` allows any length. The maxLength attribute is not required when specifying type=out.

### Attribute: `scale`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Number of decimal places in a numeric parameter. A scale of `0` allows any number of decimal places.

### Attribute: `null`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Whether the parameter is passed in as a null value. Not used with "OUT" `type` parameters.

### Attribute: `dbVarName`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: This attribute is used to specify named parameters when calling a stored procedure. If used, this attribute should be present with each cfprocparam tag of the stored procedure.
This attribute was previously deprecated then reintroduced in CF11
Databases need a variable prefix for named parameters:
`:` for Oracle
'@` for SQL Server.
See the following blog post for more information: https://coldfusion.adobe.com/2015/07/coldfusion-11-and-dbvarname-attribute/.

## Limitations

- **Must be nested inside**: `cfstoredproc`
- **Must not be nested inside**: *None*

