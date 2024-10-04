# Function Name: `CacheIdExists`

## Description
Used to find if a cached object exists in the cache region. The region can be the default cache region (either at server or application level) or the custom region you specify.

## Return Type
`boolean`

## Syntax
```cfml
cacheIdExists(id [, region])
```

## Arguments

### Argument: `id`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The ID of the cached object.

### Argument: `region`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The cache region where you check for the cached object.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

