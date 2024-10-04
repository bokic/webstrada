# Tag Name: `cftextinput`

## Description
Puts a single-line text entry box in a cfform tag and controls its display characteristics.

## Syntax
```cfml
<cftextinput name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name for the cftextinput control.

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Initial value to display in text control.

### Attribute: `required`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Yes: the user must enter or change text
 No

### Attribute: `range`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Minimum-maximum value range, delimited by a comma.
 Valid only for numeric data.

### Attribute: `validate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: date: verifies format mm/dd/yy.
 eurodate: verifies date format dd/mm/yyyy.
 time: verifies time format hh:mm:ss.
 float: verifies floating point format.
 integer: verifies integer format.
 telephone: verifies telephone format ###-###-####. The
 separator can be a blank. Area code and exchange must
 begin with digit 1 - 9.
 zipcode: verifies, in U.S. formats only, 5- or 9-digit
 format #####-####. The separator can be a blank.
 creditcard: strips blanks and dashes; verifies number using
 mod10 algorithm. Number must have 13-16 digits.
 social_security_number: verifies format ###-##-####. The
 separator can be a blank.
 regular_expression: matches input against pattern
 attribute.

### Attribute: `onvalidate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to validate user input. The form
 object, input object, and input object value are passed to
 routine, which should return True if validation succeeds,
 False otherwise. The validate attribute is ignored.

### Attribute: `pattern`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript regular expression pattern to validate input.
 Omit leading and trailing slashes

### Attribute: `message`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Message text to display if validation fails

### Attribute: `onerror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to execute if validation fails.

### Attribute: `size`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Number of characters displayed before horizontal scroll
 bar displays.

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

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Tree control height, in pixels.

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Tree control width, in pixels.

### Attribute: `vspace`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Vertical margin above and below tree control, in pixels.

### Attribute: `hspace`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Horizontal spacing to left and right of tree control, in pixels.

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: * top
 * left
 * bottom
 * baseline
 * texttop
 * absbottom
 * middle
 * absmiddle
 * right

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

### Attribute: `maxlength`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The maximum length of text entered.

### Attribute: `notsupported`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `<b>Browser must support Java to <br>view ColdFusion Java Applets!</b>`
- **Description**: Text to display if a page that contains a Java applet-based
 cfform control is opened by a browser that does not
 support Java or has Java support disabled.

### Attribute: `label`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Label for text input

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

