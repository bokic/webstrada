# Function Name: `ORMEvictQueries`

## Description
This will remove all the queries from the named query cache. If name is not specified, all queries from default cache will be removed.

## Return Type
`void`

## Syntax
```cfml
ormEvictQueries([cacheName, datasource])
```

## Arguments

### Argument: `cacheName`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the cache region that you want to evict. If you do not specify the cache, the default query cache is evicted.

### Argument: `datasource`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the data source whose cache you want to evict.

## Limitations and Other Info

- **Related Functions**: `ormEvictEntity`, `ormEvictCollection`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

