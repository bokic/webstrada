# Tag Name: `cfchart`

## Description
Generates and displays a chart.

## Syntax
```cfml
<cfchart>
```

## Attributes / Variants

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `html`
- **Description**: File format in which to save graph.
`format=flash` has been deprecated in CF2016+ and removed in CF2025
For Lucee the default value is `png` and the format `html` is not supported.

### Attribute: `chartheight`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `240`
- **Description**: Chart height; integer number of pixels

### Attribute: `chartwidth`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `320`
- **Description**: Chart width; integer number of pixels

### Attribute: `scalefrom`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Y-axis minimum value; integer

### Attribute: `scaleto`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Y-axis max value; integer

### Attribute: `showxgridlines`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: yes: display X-axis gridlines

### Attribute: `showygridlines`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: yes: display Y-axis gridlines

### Attribute: `gridlines`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `10`
- **Description**: Number of grid lines to display on value axis, including
 axis; positive integer.

### Attribute: `seriesplacement`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `default`
- **Description**: Applies to charts that have more than one data series.
 Relative positions of series.

### Attribute: `foregroundcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `black`
- **Description**: color for control. For a hex value, use the form:
 textColor = "##xxxxxx", where x = 0-9 or A-F; use two hash
 signs or none.

### Attribute: `backgroundcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Color of the area between the data background and the chart
 border, around labels and around the legend. Hexadecimal
 value or supported named color. For a hex value, use the form:
 textColor = "##xxxxxx", where x = 0-9 or A-F; use two hash
 signs or none.

### Attribute: `showborder`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Whether to display a border around the chart

### Attribute: `databackgroundcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `white`
- **Description**: color for control. For a hex value, use the form:
 textColor = "##xxxxxx", where x = 0-9 or A-F; use two hash
 signs or none.

### Attribute: `font`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `arial`
- **Description**: Font of data in column.

### Attribute: `fontsize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Size of text in column.

### Attribute: `fontitalic`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: displays grid control text in italics

### Attribute: `fontbold`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: displays grid control text in bold

### Attribute: `labelformat`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `number`
- **Description**: Format for Y-axis labels.

### Attribute: `xaxistitle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: text; X-axis title

### Attribute: `yaxistitle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: text; X-axis title

### Attribute: `xaxistype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `category`
- **Description**: CF6.1+ The axis indicates the data category. Data is sorted according to the sortXAxis attribute.
 * scale The axis is numeric. All cfchartdata item attribute
 values must numeric. The X axis is automatically sorted
 numerically.

### Attribute: `yaxistype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `category`
- **Description**: CF6.1+ Currently has no effect, as Y axis is always used for data
 values. Valid values are category and scale

### Attribute: `sortxaxis`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Display column labels in alphabetic order along X-axis.
 Ignored if the xAxisType attribute is scale.

### Attribute: `show3d`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Display chart with three-dimensional appearance.

### Attribute: `xoffset`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0.1`
- **Description**: Applies if show3D="yes". Number of units by which to
 display the chart as angled, horizontally

### Attribute: `yoffset`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0.1`
- **Description**: Applies if show3D="yes". Number of units by which to
 display the chart as angled, horizontally.

### Attribute: `showlegend`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: CF8+ if chart contains more than one data series, display legend

### Attribute: `tipstyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `mouseOver`
- **Description**: Determines the action that opens a popup window to display
 information about the current chart element.
 * mouseDown: display if the user positions the cursor at the element
 and clicks the mouse. Applies only to Flash format graph files.
 * mouseOver: displays if the user positions the cursor at the element
 * none: suppresses display

### Attribute: `tipbgcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `white`
- **Description**: color for control. For a hex value, use the form:
 textColor = "##xxxxxx", where x = 0-9 or A-F; use two hash
 signs or none.

### Attribute: `showmarkers`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Applies to chartseries type attribute values line, curve
 and scatter.
 yes: display markers at data points

### Attribute: `markersize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Size of data point marker. in pixels. Integer.

### Attribute: `pieslicestyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `sliced`
- **Description**: Applies to chartseries type attribute value pie.

### Attribute: `URL`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: URL to open if the user clicks item in a data series; the
 onClick destination page.

 You can specify variables within the URL string;
 ColdFusion passes current values of the variables.
 * $VALUE$: the value of the selected row. If none, the value is an empty string.
 * $ITEMLABEL$: the label of the selected item. If none, the value is an empty string.
 * $SERIESLABEL$: the label of the selected series. If none, the value is an empty string.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Page variable name. String. Generates the graph as
 binary data and assigns it to the specified variable.
 Suppresses chart display. You can use the name value in
 the cffile tag to write the chart to a file.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF7+ XML file or string to use to specify the style of the chart.

### Attribute: `title`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF7+ Title of the chart.

### Attribute: `base64`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF2018+ Can only be used for client side charts.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

