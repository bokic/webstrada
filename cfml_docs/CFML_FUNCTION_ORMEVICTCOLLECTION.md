# Function Name: `ORMEvictCollection`

## Description
This will remove all the entries with the specified relation/collection name in the specified component.

## Return Type
`void`

## Syntax
```cfml
ormEvictCollection(entityName, collectionName [, primaryKey])
```

## Arguments

### Argument: `entityName`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Entity name of the persistent CFC

### Argument: `collectionName`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the collection in the component

### Argument: `primaryKey`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Primary key of the collection or association data of the entity

## Limitations and Other Info

- **Related Functions**: `ormEvictEntity`, `ormEvictQueries`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

