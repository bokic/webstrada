# Function Name: `CacheRegionExists`

## Description
Checks if the cache region exists.

## Return Type
`boolean`

## Syntax
```cfml
cacheRegionExists(region)
```

## Arguments

### Argument: `region`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the cache region.

### Argument: `password `
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee5+ The password for the web administrator is required to interact with Cache Connections.

## Limitations and Other Info

- **Related Functions**: `cacheRegionNew`, `cacheRegionRemove`
- **Coldfusion Support**: Minimum version: `10`.
- **Openbd Support**:
- **Lucee Support**: Minimum version: `5`. Notes: Only been added for compatibility to other CFML Engines. These functions are already marked as "deprecated" and it's strongly suggested not to use them in new code.

