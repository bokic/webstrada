# Function Name: `EntityLoadByPK`

## Description
Loads and returns an array of objects for given primary key.
Use this function to avoid specifying the required boolean parameter in `EntityLoad()`.

## Return Type
`any`

## Syntax
```cfml
entityLoadByPK(entity, id)
```

## Arguments

### Argument: `entity`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the entity to be loaded.

### Argument: `id`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: ID of the entity to be loaded.

## Limitations and Other Info

- **Related Functions**: `entityLoadByExample`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**: Notes: Allows the optional `unique` parameter.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

