# Tag Name: `cfhtmltopdfitem`

## Description
Header Footer PageBreak subdefinition for cfhtmltopdf.

You can access the following scope variables in <cfhtmltopdfitem> content:

_PAGENUMBER – Add current page number.
_LASTPAGENUMBER – Add last page number.

## Syntax
```cfml
<cfhtmltopdfitem>
```

## Attributes / Variants

### Attribute: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specifies the action:

header: uses the text/image specified between the <cfhtmltopdfitem> and </cfhtmltopdfitem> tags as the running header.
footer: uses the text/image between the <cfhtmltopdfitem> and </cfhtmltopdfitem> tags as the running footer.
pagebreak: can be used to insert a pagebreak in the generated PDF. When <cfhtmltopdf> is used as a service, pagebreak will not work. 

### Attribute: `isBase64`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Use this only when the image attribute is given a base64 image string

### Attribute: `image`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The image file name or object to be used as header or footer.

The following parameters are supported:

A path to the image file
A Base-64 string
A byte array
A <cfimage> object

### Attribute: `opacity`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `10`
- **Description**: Opacity of the header/footer. Specify a valid number. A number between 1 and 10.

### Attribute: `numberformat`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `Numeric`
- **Description**: The page number format to be used. 

### Attribute: `showonprint`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Show or hide header/footer when the document is printed.

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `Center`
- **Description**: Alignment of the text. Left, Right, or Center.

### Attribute: `leftmargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1.0`
- **Description**: Left margin in inches (default) or centimeters. To specify the left margin in centimeters, include the unit=cm attribute.

### Attribute: `rightmargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1.0`
- **Description**: Right margin in inches (default) or centimeters. To specify the left margin in centimeters, include the unit=cm attribute.

### Attribute: `topmargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0.5`
- **Description**: Top margin in inches (default) or centimeters. To specify the left margin in centimeters, include the unit=cm attribute.

### Attribute: `bottommargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0.5`
- **Description**: Bottom margin in inches (default) or centimeters. To specify the bottom margin in centimeters, include the unit=cm attribute.

## Limitations

- **Must be nested inside**: `cfhtmltopdf`
- **Must not be nested inside**: *None*

