# Tag Name: `cfstoredproc`

## Description
Executes a stored procedure in a server database. It
 specifies database connection information and identifies
 the stored procedure.

## Syntax
```cfml
<cfstoredproc procedure="">
```

## Attributes / Variants

### Attribute: `procedure`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of stored procedure on database server.

### Attribute: `datasource`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of data source that points to database that contains
 stored procedure.

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides username in data source setup.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides password in data source setup.

### Attribute: `blockfactor`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum number of rows to get at a time from server.
 Range is 1 to 100.

### Attribute: `debug`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: Lists debug information on each statement
 No: does not

### Attribute: `returncode`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: Tag populates cfstoredproc.statusCode with status
 code returned by stored procedure.
 No: does not

### Attribute: `result`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies a name for the structure in which cfstoredproc
 returns the statusCode and ExecutionTime variables. If
 set, this value replaces cfstoredproc as the prefix to
 use when accessing those variables.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

