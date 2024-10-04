# Function Name: `CacheGetProperties`

## Description
Gets the cache properties for the object cache, the page cache, or both. The information is application-specific.

## Return Type
`array`

## Syntax
```cfml
cacheGetProperties(region)
```

## Arguments

### Argument: `region`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ CF10+ Properties to get information for

"" or not set - information to all default caches
object - information to "Default Object" Cache
template - information to "Default Template" Cache
query - information to "Default Query" Cache
resource - information to "Default Resource" Cache
{cache name} - information to a specific cache

## Limitations and Other Info

- **Related Functions**: `cacheSetProperties`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

