# Function Name: `EntitySave`

## Description
Saves or updates data of the entity and all related entities to the database.

## Return Type
`void`

## Syntax
```cfml
entitySave(entity [, forceInsert])
```

## Arguments

### Argument: `entity`
- **Type**: `variableName`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the entity that must be saved in the database.

### Argument: `forceInsert`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: If true, then ColdFusion always tries to insert the entity as a new record.

## Limitations and Other Info

- **Related Functions**: `entityToQuery`, `entityReload`, `entityMerge`, `entityDelete`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

