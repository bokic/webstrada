# Function Name: `ORMEvictEntity`

## Description
This will remove all the entries for the specified component name from the entity cache.

## Return Type
`void`

## Syntax
```cfml
ormEvictEntity(entityName [, primaryKey])
```

## Arguments

### Argument: `entityName`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Entity name of the persistent CFC

### Argument: `primaryKey`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Primary key value of the component

## Limitations and Other Info

- **Related Functions**: `ormEvictCollection`, `ormEvictQueries`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

