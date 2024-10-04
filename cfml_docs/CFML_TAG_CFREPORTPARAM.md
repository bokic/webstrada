# Tag Name: `cfreportparam`

## Description
Passes input parameters to a ColdFusion Report Builder
 report definition. Allowed inside cfreport tag bodies only.

## Syntax
```cfml
<cfreportparam>
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Variable name for data that is passed. The ColdFusion
 Report Builder report definition must include an input
 parameter that matches this name.

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Value of the data that is sent.

### Attribute: `chart`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the chart contained in a report or subreport. The value of this attribute must match Name property of a chart defined in the Report Builder report.

### Attribute: `query`
- **Type**: `query`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Query value to pass to a subreport or chart. The ColdFusion query must contain at least all of the columns included in the Report Builder query.

### Attribute: `series`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Ordinal number of a chart series to use for the query. This attribute is valid only when the chart attribute is specified.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Style in CSS format for a subreport.

### Attribute: `subreport`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the subreport.

## Limitations

- **Must be nested inside**: `cfreport`
- **Must not be nested inside**: *None*

