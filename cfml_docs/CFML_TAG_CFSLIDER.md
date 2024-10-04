# Tag Name: `cfslider`

## Description
Puts a slider control, for selecting a numeric value from a
 range, in a ColdFusion form. The slider moves over the slider
 groove. As the user moves the slider, the current value
 displays. Used within a cfform tag.
 Not supported with Flash forms.

## Syntax
```cfml
<cfslider name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name for cfslider control.

### Attribute: `label`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Label to display with control.
 For example, "Volume" This displays: "Volume %value%"
 To reference the value, use "%value%". If %% is omitted,
 slider value displays directly after label.

### Attribute: `range`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `0,100`
- **Description**: Numeric slider range values.
 Separate values with a comma.

### Attribute: `scale`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Unsigned integer. Defines slider scale, within range.
 For example: if range = "0,1000" and scale = "100",
 the display values are: 0, 100, 200, 300, ...
 Signed and unsigned integers in ColdFusion are in the
 range -2,147,483,648 to 2,147,483,647.

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Starting slider setting. Must be within the range values.

### Attribute: `onvalidate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to validate user input; in this
 case, a change to the default slider value. Specify only
 the function name.

### Attribute: `message`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Message text to appear if validation fails.

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `40`
- **Description**: Slider control height, in pixels.

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Slider control width, in pixels.

### Attribute: `vspace`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Vertical spacing above and below slider, in pixels.

### Attribute: `hspace`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Horizontal spacing to left and right of slider, in pixels.

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Alignment of slider:
 * top
 * left
 * bottom
 * baseline
 * texttop
 * absbottom
 * middle
 * absmiddle
 * right

### Attribute: `lookandfeel`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `windows`
- **Description**: - motif: renders slider in Motif style
 - windows: renders slider in Windows style
 - metal: renders slider in Java Swing style
 If platform does not support style option, tag defaults to
 platform default style.
 Default: windows

### Attribute: `vertical`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: Renders slider in browser vertically. You must set
 width and height attributes; ColdFusion does not
 automatically swap width and height values.
 No: Renders slider horizontally.

### Attribute: `bgcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Background color of control. For a hex value, use the form:
 textColor = "##xxxxxx", where x = 0-9 or A-F; use two hash
 signs or none.

### Attribute: `textcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text color for control. For a hex value, use the form:
 textColor = "##xxxxxx", where x = 0-9 or A-F; use two hash
 signs or none.

### Attribute: `font`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `arial`
- **Description**: Font name for data in tree control.

### Attribute: `fontsize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Font size for text in tree control, in points.

### Attribute: `italic`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: displays tree control text in italics
 No: it does not

### Attribute: `bold`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: displays tree control text in bold
 No: it does not

### Attribute: `notsupported`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `<b>Browser must support Java to <br>view ColdFusion Java Applets!</b>`
- **Description**: Text to display if a page that contains a Java applet-based
 cfform control is opened by a browser that does not
 support Java or has Java support disabled.

### Attribute: `clickToChange`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether clicking the slider changes the value of the pointer:

### Attribute: `max`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum value for the slider.

### Attribute: `onChange`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to run when slider value changes.
Specify only the function name.

### Attribute: `min`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Minimum value for the slider.

### Attribute: `onDrag`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to run when you drag the slider.
Specify only the function name.

### Attribute: `onError`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to run if validation fails.
Specify only the function name.

### Attribute: `increment`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The unit increment value for a snapping slider.

### Attribute: `tip`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether the data value has to display as data tips

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies if the format is:html/applet

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

