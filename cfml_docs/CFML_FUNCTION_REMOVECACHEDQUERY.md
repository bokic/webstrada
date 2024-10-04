# Function Name: `RemoveCachedQuery`

## Description
 Removes the query with the details you provide from query cache.

## Return Type
`void`

## Syntax
```cfml
removeCachedQuery(SQL_, datasource, params, region___);
```

## Arguments

### Argument: `SQL`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The Query SQL.

### Argument: `datasource`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The datasource you ran the query on.

### Argument: `params`
- **Type**: `array`
- **Required**: Optional
- **Default Value**: *None*
- **Description**:  Array of parameter values passed to SQL.

### Argument: `region`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the cache region where you can place the cache object.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `10`.

