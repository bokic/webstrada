# Tag Name: `cftransaction`

## Description
Instructs the database management system to treat multiple
 database operations as a single transaction. Provides database
 commit and rollback processing.
Note that distributed transactions (transactions across multiple datasources) are not supported - you must commit one transaction and begin a separate transaction to one database before writing a query to another (CFMX7 Manual)

## Syntax
```cfml
<cftransaction>
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `begin`
- **Description**: `begin`: the start of the block of code to execute
`commit`: commits a pending transaction
`rollback`: rolls back a pending transaction
`setsavepoint`: Marks a place within the transaction as a savepoint.

### Attribute: `isolation`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: ODBC lock type.

### Attribute: `savepoint`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the savepoint in the transaction. Used with `action="setsavepoint"` or `action="rollback"`

### Attribute: `nested`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: specifies whether transaction is nested or not

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

