# Tag Name: `cfgridrow`

## Description
Lets you define a cfgrid that does not use a query as source
 for row data. If a query attribute is specified in cfgrid, the
 cfgridrow tags are ignored.

## Syntax
```cfml
<cfgridrow data="">
```

## Attributes / Variants

### Attribute: `data`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Delimited list of column values. If a value contains a
 comma, it must be escaped with another comma

### Attribute: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Delimiter to be used for data

## Limitations

- **Must be nested inside**: `cfgrid`
- **Must not be nested inside**: *None*

