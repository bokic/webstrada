# Function Name: `CacheRemoveAll`

## Description
Removes all stored objects in a cache region. If no cache region is specified, objects in the default region are removed.

## Return Type
`void`

## Syntax
```cfml
cacheRemoveAll( region )
```

## Arguments

### Argument: `region`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Indicates the cache region from which to remove the stored objects. If no value is specified, default cache region is considered by default.

## Limitations and Other Info

- **Related Functions**: `cacheDelete`, `cacheClear`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

