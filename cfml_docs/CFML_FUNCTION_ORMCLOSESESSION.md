# Function Name: `ORMCloseSession`

## Description
Closes the current ORM session for the given datasource. When the datasource argument is not passed then it uses the default datasource defined in Application.cfc.

## Return Type
`void`

## Syntax
```cfml
ormCloseSession([datasource])
```

## Arguments

### Argument: `datasource`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the datasource for the ORM session

## Limitations and Other Info

- **Related Functions**: `ormGetSession`, `ormClearSession`, `ormFlush`, `ormGetSessionFactory`, `ormCloseAllSession`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

