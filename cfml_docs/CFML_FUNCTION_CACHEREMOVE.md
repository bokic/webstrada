# Function Name: `CacheRemove`

## Description
Removes an object from the cache.

## Return Type
`void`

## Syntax
```cfml
cacheRemove(id [, throwOnError [, region[, exact]]])
```

## Arguments

### Argument: `id`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Comma delimited list of cache IDs. A list of all available IDs can be retrieved using cacheGetAllIds. CF11+ Can take an array instead of a list.

### Argument: `throwOnError`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: When `true` throws an error when cache ID does not exist.

### Argument: `region`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ Specify which cache region to search

### Argument: `exact`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: CF10+ When `true`, matches values in `id` exactly

## Limitations and Other Info

- **Related Functions**: `cacheRemoveAll`, `cacheGetAllIds`, `cachePut`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

