# Function Name: `EntityDelete`

## Description
Deletes the record from the database for the specified entity. Depending on the cascade attribute specified in the mapping, it deletes the associated objects also.

## Return Type
`void`

## Syntax
```cfml
entityDelete(entity)
```

## Arguments

### Argument: `entity`
- **Type**: `variableName`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the entity being deleted.

## Limitations and Other Info

- **Related Functions**: `entitySave`, `entityReload`, `entityMerge`, `entityToQuery`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

