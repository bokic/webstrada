# Function Name: `CacheGetSession`

## Description
Lets you retrieve the underlying cache object to access additional cache functionality that is not implemented in the tag cfcache.

Note: Caution! Using the cacheGetSession function might pose security vulnerabilities. If you wish to disable the usage of this function, add it to Sandbox Security. 

## Return Type
`any`

## Syntax
```cfml
cacheGetSession(objectType)
```

## Arguments

### Argument: `objectType`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Any of the following values: object, template, or name of the user-defined cache

### Argument: `isKey`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Set to true if objectType is user-defined cache.

## Limitations and Other Info

- **Related Functions**: `cfcache`, `cachePut`, `cacheGet`, `cacheGetAllIds`, `cacheGetMetadata`, `cacheGetProperties`, `cacheSetProperties`
- **Coldfusion Support**: Minimum version: `9.0.1`.

