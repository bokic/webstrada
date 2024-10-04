# Function Name: `EntityNew`

## Description
Creates a new instance of the persistent CFC with the entity name that you provide.

## Return Type
`any`

## Syntax
```cfml
entityNew(entityName [,properties [,ignoreExtras]])
```

## Arguments

### Argument: `entityName`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Entity name of the persistent CFC.

### Argument: `properties`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Key-value pair (CF struct) of property names and values.

### Argument: `ignoreExtras`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: No Help Available

## Limitations and Other Info

- **Related Functions**: `entityDelete`
- **Coldfusion Support**: Minimum version: `9`. Notes: This function has been enhanced in ColdFusion 9.0.1 to support multiple data sources in the same application.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

