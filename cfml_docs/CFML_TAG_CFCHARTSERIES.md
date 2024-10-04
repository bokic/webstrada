# Tag Name: `cfchartseries`

## Description
Used with the cfchart tag. This tag defines the style in which
 chart data displays: bar, line, pie, and so on.

## Syntax
```cfml
<cfchartseries type="bar">
```

## Attributes / Variants

### Attribute: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Sets the chart display style

### Attribute: `query`
- **Type**: `query`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of CFML query from which to get data.

### Attribute: `itemcolumn`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of a column in the query specified in the query
 attribute; contains the item label for a data point to
 graph.

### Attribute: `valuecolumn`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of a column in the query specified in the query
 attribute; contains data values to graph.

### Attribute: `serieslabel`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text of data series label

### Attribute: `seriescolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Color of the main element (such as the bars) of a chart.
 For a pie chart, the color of the first slice.

 Hex value or supported named color; see name list in the
 cfchart Usage section.
 For a hex value, use the form "##xxxxxx", where x = 0-9 or
 A-F; use two pound signs or none.

### Attribute: `paintstyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `plain`
- **Description**: Sets the paint display style of the data series.

### Attribute: `markerstyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `rectangle`
- **Description**: Applies to chartseries type attribute values line, curve
 and scatter, and show3D attribute value no.

### Attribute: `colorlist`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies if chartseries type attribute = "pie". Sets pie
 slice colors.

 Comma-delimited list of hex values or supported, named web
 colors; see name list in the cfchart Usage section.

 For a hex value, use the form "##xxxxxx", where x = 0-9 or
 A-F; use two pound signs or none.

### Attribute: `datalabelstyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the way in which the color is applied to the
 item in the series:
 - None = nothing is printed (default)
 - Value = the value of the datapoint
 - Rowlabel = the row's label
 - Columnlabel = the column's label
 - Pattern

## Limitations

- **Must be nested inside**: `cfchart`
- **Must not be nested inside**: *None*

