# Function Name: `CachePut`

## Description
Stores an object in the cache.

## Return Type
`void`

## Syntax
```cfml
cachePut(id, value [, timespan] [, idleTime] [, region] [, throwOnError])
```

## Arguments

### Argument: `id`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Unique identifier for the cached value

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The value to cache

### Argument: `timespan`
- **Type**: `date`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The interval until the object is flushed from the cache, as a decimal number of days. One way to set the value is to use the return value from the CreateTimeSpan function. The default is to not time out the object.

### Argument: `idleTime`
- **Type**: `date`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A decimal number of days after which the object is flushed from the cache if it is not accessed during that time. One way to set the value is to use the return value from the CreateTimeSpan function.

### Argument: `region`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ CF10+ Specifies the cache region/name where you place the cache object.

### Argument: `throwOnError`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF10+ If true and region does not exist, throws an error

## Limitations and Other Info

- **Related Functions**: `cacheGet`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

