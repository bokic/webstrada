# Function Name: `ORMFlush`

## Description
Flushes the current ORM session of all the pending CRUD operations in that request. Any changes made in the objects, in the current ORM session, are saved to the database.

## Return Type
`void`

## Syntax
```cfml
ormFlush([datasource])
```

## Arguments

### Argument: `datasource`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the data source used for the session. If not defined, the default datasource defined in Application.cfc/cfapplication is used.

## Limitations and Other Info

- **Related Functions**: `ormFlushAll`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

