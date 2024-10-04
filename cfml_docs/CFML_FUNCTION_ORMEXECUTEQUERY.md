# Function Name: `ORMExecuteQuery`

## Description
Runs the HQL on the default data source specified for the application.

## Return Type
`any`

## Syntax
```cfml
ormExecuteQuery(hql, params [,unique]);
ormExecuteQuery(hql, [,unique] [, queryoptions]);
ormExecuteQuery(hql, params [,unique] [,queryOptions])
```

## Arguments

### Argument: `hql`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A HQL query statement

### Argument: `params`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A struct or array of query params.

### Argument: `unique`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If true returns a single entity instead of an array.

### Argument: `queryoptions`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A struct with possible keys: ignorecase, maxResults, offset, cacheable, timeout, datasource

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

