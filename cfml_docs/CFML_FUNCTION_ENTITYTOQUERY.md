# Function Name: `EntityToQuery`

## Description
Converts the input entity object or the input array of entity objects to a query object.
The following conditions apply for this function:
In the case of array input, all objects in the array must be of the same type.
The result query will not contain any relation data.

## Return Type
`query`

## Syntax
```cfml
entityToQuery(entity [, name])
```

## Arguments

### Argument: `entity`
- **Type**: `variableName`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Entity object or array of objects that needs to be converted to a query object.

### Argument: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the entity. Use this optional parameter to return the query of the given entity in the case of inheritance mapping.

## Limitations and Other Info

- **Related Functions**: `entitySave`, `entityReload`, `entityMerge`, `entityDelete`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

