# Tag Name: `cfsprydataset`

## Description
Creates a Spry data set; can use bind parameters to get data from ColdFusion AJAX controls 
 to populate the data set.

## Syntax
```cfml
<cfsprydataset bind="" name="">
```

## Attributes / Variants

### Attribute: `bind`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A bind expression that returns XML data to populate 
 the Spry data set. The bind expression can specify a 
 CFC function or URL and can include bind parameters 
 that represent the values of ColdFusion controls. For 
 detailed information on specifying bind expressions, 
 see HTML form data binding in cfinput.

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of the Spry data set.

### Attribute: `onbinderror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a JavaScript function to execute if 
 evaluating the bind expression results in an error. The 
 function must take two attributes: an HTTP status 
 code and a message. 
 If you omit this attribute, and have specified a global 
 error handler (by using the 
 ColdFusion.setGlobalErrorHandler function), the 
 handler displays the error message; otherwise a 
 default error pop-up displays.

### Attribute: `options`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A JavaScript object containing options to pass to the 
 data set.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies data set type, corresponding to the format of 
 the data that is returned by the bind expression.

### Attribute: `xpath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Valid for XML type data sets only. An XPath 
 expression that extracts data from the XML returned 
 by the bind expression. The data set contains only the 
 data that matches the xpath expression.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

