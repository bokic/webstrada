# Function Name: `CacheRegionNew`

## Description
Creates a new custom cache region (if no cache region exists).

## Return Type
`void`

## Syntax
```cfml
cacheRegionNew(region [, properties] [, throwOnError])
```

## Arguments

### Argument: `region`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the new cache region to be created.

### Argument: `properties`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Struct that contains the cache region properties.

### Argument: `throwOnError`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: A Boolean value specifying if to throw an exception if the cache region name you specify already exists.

### Argument: `password `
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee5+ The password for the web administrator is required to interact with Cache Connections.

## Limitations and Other Info

- **Related Functions**: `cacheRegionExists`
- **Coldfusion Support**: Minimum version: `10`.
- **Openbd Support**:
- **Lucee Support**: Minimum version: `5`. Notes: Only been added for compatibility to other CFML Engines. These functions are already marked as "deprecated" and it's strongly suggested not to use them in new code.

