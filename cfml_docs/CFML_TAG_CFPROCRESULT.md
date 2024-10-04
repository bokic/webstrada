# Tag Name: `cfprocresult`

## Description
Associates a query object with a result set returned by a
 stored procedure. Other CFML tags, such as cfoutput and
 cftable, use this query object to access the result set. This
 tag is nested within a cfstoredproc tag.

## Syntax
```cfml
<cfprocresult name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name for the query result set.

### Attribute: `resultset`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Names one result set, if stored procedure returns more than
 one.

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum number of rows returned in result set.

## Limitations

- **Must be nested inside**: `cfstoredproc`
- **Must not be nested inside**: *None*

