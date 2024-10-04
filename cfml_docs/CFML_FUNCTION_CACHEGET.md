# Function Name: `CacheGet`

## Description
Gets an object that is stored in the cache.

## Return Type
`any`

## Syntax
```cfml
cacheGet(id [,region])
```

## Arguments

### Argument: `id`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The ID value assigned to the cache object when it was created

### Argument: `region`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ The name of the cache region where the object was stored. Applies only to ACF.

### Argument: `cacheName`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ The name of the cache where the object was stored. Applies only to Lucee.

### Argument: `throwWhenNotExist`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Lucee4.5+ Enable/Disable throwing an error if element not exists

## Limitations and Other Info

- **Related Functions**: `cachePut`, `cacheGetAllIds`, `cacheGetMetadata`, `cacheGetProperties`, `cacheRemove`, `cacheSetProperties`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**: Notes: For Lucee the method signature is:
`cacheGet( id [, throwWhenNotExist [, cacheName ] ] )`
however it does support passing a cacheName in as the second argument for compatibility with ACF.
- **Railo Support**:
- **Openbd Support**:

