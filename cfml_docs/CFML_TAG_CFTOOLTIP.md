# Tag Name: `cftooltip`

## Description
Specifies tool tip text that displays when the user hovers the mouse pointer over the elements 
 in the tag body. This tag does not require a form and is not used inside Flash forms.

## Syntax
```cfml
<cftooltip>
```

## Attributes / Variants

### Attribute: `preventoverlap`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: A Boolean value specifying whether to prevent the 
 tooltip from overlapping the component that it 
 describes.

### Attribute: `autodismissdelay`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The number of milliseconds between the time 
 when the user moves the mouse pointer over the 
 component (and leaves it there) and when the 
 tooltip disappears.

### Attribute: `hidedelay`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The number of milliseconds to delay between the 
 time when the user moves the mouse pointer away 
 from the component and when the tooltip 
 disappears.

### Attribute: `showdelay`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The number of milliseconds to delay between the 
 time when the user moves the mouse over the 
 component and when the tooltip appears.

### Attribute: `sourcefortooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The URL of a page with the tool tip contents. The 
 page can include HTML markup to control the 
 format, and the tip can include images. 
 If you specify this attribute, an animated icon 
 appears with the text "Loading..." while the tip is 
 being loaded.

### Attribute: `tooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Tip text to display. The text can include HTML 
 formatting. 
 Ignored if you specify a sourceForTooltip attribute

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: 

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

