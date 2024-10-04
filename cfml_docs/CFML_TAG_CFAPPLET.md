# Tag Name: `cfapplet`

## Description
This tag references a registered custom Java applet. To
 register a Java applet, in the CFML Administrator, click
 Extensions/Java Applets.
 Using this tag within a cfform tag is optional. If you use it
 within cfform, and the method attribute is defined in the
 Administrator, the return value is incorporated into the
 form.

## Syntax
```cfml
<cfapplet appletsource="" name="">
```

## Attributes / Variants

### Attribute: `appletsource`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of registered applet

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Form variable name for applet

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Control's height, in pixels.

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Control's width, in pixels.

### Attribute: `vspace`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Vertical margin above and below control, in pixels.

### Attribute: `hspace`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Horizontal spacing to left and right of control, in pixels.

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Alignment

### Attribute: `notsupported`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `<b>Browser must support Java to <br>view ColdFusion Java Applets!</b>`
- **Description**: Text to display if a page that contains a Java applet-based
 cfform control is opened by a browser that does not
 support Java or has Java support disabled.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

