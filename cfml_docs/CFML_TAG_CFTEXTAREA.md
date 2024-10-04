# Tag Name: `cftextarea`

## Description
Puts a multiline text entry box in a cfform tag and
 controls its display characteristics.

## Syntax
```cfml
<cftextarea name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the cftextinput control.

### Attribute: `label`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Label to put beside the control on a form.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: In HTML or XML format forms, ColdFusion passes the
 style attribute to the browser or XML.
 In Flash format forms, must be a style specification in
 CSS format, with the same syntax and contents as used
 in Macromedia Flex for the corresponding Flash element.

### Attribute: `required`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: - true: the field must contain text.
 - false: the field can be empty.
 Default: false

### Attribute: `html`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Boolean value that specifies whether the text area contains HTML.
 Default: false

### Attribute: `validate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The type or types of validation to do. Available validation
 types and algorithms depend on the format. For details,
 see the Usage section of the cfinput tag reference.

### Attribute: `validateat`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `onSubmit`
- **Description**: How to do the validation. For Flash format forms, onSubmit
 and onBlur are identical; validation is done on submit.
 For multiple values, use a comma-delimited list.
 For details, see the Usage section of the cfinput tag
 reference.
 Default: onSubmit

### Attribute: `message`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Message text to display if validation fails.

### Attribute: `range`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Minimum and maximum numeric allowed values. ColdFusion
 uses this attribute only if you specify range in the
 validate attribute.
 If you specify a single number or a single number a
 followed by a comma, it is treated as a minimum, with no
 maximum. If you specify a comma followed by a number,
 the maximum is set to the specified number, with no
 minimum.

### Attribute: `maxlength`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The maximum length of text that can be entered.
 ColdFusion uses this attribute only if you specify
 maxlength in the validate attribute.

### Attribute: `pattern`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript regular expression pattern to validate input.
 Omit leading and trailing slashes. ColdFusion uses this
 attribute only if you specify regex in the validate attribute.

### Attribute: `onvalidate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to validate user input. The
 JavaScript DOM form object, input object, and input
 object value are passed to routine, which should return
 True if validation succeeds, False otherwise. If you specify
 this attribute, ColdFusion ignores the validate attribute.

### Attribute: `onerror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to execute if validation fails.

### Attribute: `disabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Disables user input, making the control read-only. To
 disable input, specify disabled without an attribute, or
 disabled="true". To enable input, omit the attribute or
 specify disabled="false".
 Default: false

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Initial value to display in text control. You can specify an
 initial value as an attribute or in the tag body, but not in
 both places. If you specify the value as an attribute, you
 must put the closing cftextarea tag immediately after the
 opening cftextarea tag, with no spaces or line feeds between,
 or place a closing slash at the end of the opening cftextarea
 tag. For example:
 <cftextarea name="description" value="Enter a description." />

### Attribute: `bind`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: A Flex bind expression that populates the field with
 information from other form fields.

### Attribute: `onkeyup`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash) to run
 when the user releases a keyboard key in the control.

### Attribute: `onkeydown`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash)
 ActionScript to run when the user presses a keyboard
 key in the control.

### Attribute: `onmouseup`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash) to run
 when the user presses a mouse button in the control.

### Attribute: `onmousedown`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash) to run
 when the user releases a mouse button in the control.

### Attribute: `onchange`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash) to run
 when the control changes due to user action.

### Attribute: `onclick`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) to run when the user clicks the
 control. Not supported for Flash forms.

### Attribute: `enabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying whether the control is enabled.
 A disabled control appears in light gray. The inverse of the
 disabled attribute.
 Default: true

### Attribute: `visible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying whether to
 show the control. Space that would be occupied by an
 invisible control is blank.
 Default: true

### Attribute: `tooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: Text to display when the mouse pointer hovers
 over the control.

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: The height of the control, in pixels.

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: The width of the control, in pixels.

### Attribute: `basepath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Path to the directory that contains the rich text editor. The editor configuration files are at the top level of this directory.

### Attribute: `bindattribute`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the HTML tag attribute whose value is set by the bind attribute. You can only specify attributes in the browser's HTML DOM tree, not ColdFusion-specific attributes.

### Attribute: `bindonload`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value that specifies whether to execute the bind attribute expression when first loading the form.

### Attribute: `fontformats`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A comma separated list of the font names to display in the rich text editor Formats selector. The formats specify the HTML tags to apply to typed or selected text.

### Attribute: `fontnames`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A comma separated list of the font names to display in the rich text editor Font selector.

### Attribute: `fontsizes`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A comma separated list of the font sizes to display in the rich text editor Size selector. List entries must have the format of numeric font size/descriptive text.

### Attribute: `onbinderror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a JavaScript function to execute if evaluating a bind expression results in an error. The function must take two attributes: an HTTP status code and a message.

### Attribute: `richtext`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value specifying whether this control is a rich text editor with tool bars to control the text formatting.

### Attribute: `skin`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `default`
- **Description**: Specifies the skin to be used for the rich text editor. By default, the valid values are Default, silver, and office2003. 
You can also create custom skins that you can then specify in this attribute.

### Attribute: `wrap`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: * hard: wraps long lines, and sends the carriage return to the server.
 * off: does not wrap long lines.
 * physical: wraps long lines, and transmits the text at all wrap points.
 * soft: wraps long lines, but does not send the carriage return to the server.
 * virtual: wraps long lines, but does not send the carriage return to the server.

### Attribute: `sourcefortooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The URL of a page to display as a tool tip. 
The page can include CFML and HTML to control the contents and format, and the tip can include images.
If you specify this attribute, an animated icon appears with the text "Loading..." while the tip is being loaded.

### Attribute: `stylesxml`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The path of the file that defines the styles in the rich text editor Styles selector. 
Relative paths start at the directory that contains the fckeditor.html file, normally cf_webRoot/CFIDE/scripts/ajax/FCKeditor/editor.

### Attribute: `templatesxml`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The pathof the file that defines the templates that are displayed when you click the rich text editor Templates icon.

### Attribute: `toolbar`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the rich text editor toolbar. By default, the valid values for this attribute are: Default, a complete set of controls, and Basic, a minimal configuration.

### Attribute: `toolbaronfocus`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value that specifies whether the rich text editor toolbar expands and displays its controls only when the rich text editor has the focus.

### Attribute: `secureUpload`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If true, enables secure upload using FCKeditor.
For secure upload, you must have
sessionManagement set to yes. Secure upload does not work if sessionManagement is set to false.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

