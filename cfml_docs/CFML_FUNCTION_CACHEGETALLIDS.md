# Function Name: `CacheGetAllIds`

## Description
This function return an array containing all keys inside the cache.

## Return Type
`array`

## Syntax
```cfml
cacheGetAllIds()
```

## Arguments

### Argument: `filter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ Key filter for the elements, the filter follow the same rules as for cfdirectory-filter.

### Argument: `cacheName`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `ObjectCache`
- **Description**: CF10+ The cache region to use or the default object cache

### Argument: `isAccurate`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: CF11+ When set to false, function will return the result faster. However, the result may not be accurate. If you need only the IDs of valid (unexpired) objects from the cache, set accurate to true. If you set accurate to false, the IDs of all the objects in the cache will be returned.

## Limitations and Other Info

- **Related Functions**: `cacheGet`, `cacheGetAll`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:

