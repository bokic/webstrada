# Function Name: `ORMClearSession`

## Description
ORMClearSession removes all the entities that are loaded or created in the session. This clears the first level cache and removes the objects that are not yet saved to the database.

## Return Type
`void`

## Syntax
```cfml
ormClearSession([datasource])
```

## Arguments

### Argument: `datasource`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the data source used for the session. If not defined, the default datasource defined in Application.cfc/cfapplication is used.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

