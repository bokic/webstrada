# Function Name: `QueryExecute`

## Description
Executes a SQL query, returns the result.

## Return Type
`query`

## Syntax
```cfml
queryExecute(sql [, params, options])
```

## Arguments

### Argument: `sql`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: SQL string to execute.

### Argument: `params`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Array or Struct of parameter values. When passing an array use ? as place holders. When passing a struct use :keyName where keyName is the name of the key in the structure corresponding to the parameter. The struct or array can be a struct with keys such as the following.

### Argument: `options`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Struct containing query options, all cfquery tag attributes are supported except the name attribute.

## Limitations and Other Info

- **Type Requirement**: Operates on query recordsets.
- **Related Functions**: `cfquery`, `cfqueryparam`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**: Minimum version: `4.5`.
- **Boxlang Support**: Minimum version: `1.0.0`.

