# Tag Name: `cfchartdata`

## Description
Used with the cfchart and cfchartseries tags. This tag defines
 chart data points. Its data is submitted to the cfchartseries
 tag.

## Syntax
```cfml
<cfchartdata item="" value="">
```

## Attributes / Variants

### Attribute: `item`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: string; data point name

### Attribute: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: number or expression; data point value

## Limitations

- **Must be nested inside**: `cfchart`, `cfchartseries`
- **Must not be nested inside**: *None*

