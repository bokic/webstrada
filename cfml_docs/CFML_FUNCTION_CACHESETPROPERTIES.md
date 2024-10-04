# Function Name: `CacheSetProperties`

## Description
Set multiple cache settings

## Return Type
`void`

## Syntax
```cfml
cacheSetProperties(properties)
```

## Arguments

### Argument: `properties`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Structure of properties to be changed

### Argument: `region`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ Indicates the cache region for which to set the properties.

## Limitations and Other Info

- **Related Functions**: `cacheGetProperties`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**: Notes: This function is not supported by Lucee, because they see this as a security risk, as you can modify cache configuration without a password, you can use tag cfadmin instead. Lucee will add support for this function in a future release.
- **Railo Support**:
- **Openbd Support**:

