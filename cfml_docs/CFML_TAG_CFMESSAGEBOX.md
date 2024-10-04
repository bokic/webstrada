# Tag Name: `cfmessagebox`

## Description
Creates MessageBox

## Syntax
```cfml
<cfmessagebox type="alert" message="" name="">
```

## Attributes / Variants

### Attribute: `labelcancel`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The text to put on the cancel button of a prompt message box.

### Attribute: `callbackHandler`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: callback function which will be called when any ok|no|cancel button is clicked

### Attribute: `labelok`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The text to put on an alert button and prompt message box OK button.

### Attribute: `title`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The title for the message box.

### Attribute: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Type of action to be performed

### Attribute: `message`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Message text which will be displayed

### Attribute: `labelno`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The text to put on the button used for a negative response in a confirm message box.

### Attribute: `labelyes`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The text to put on the button used for a positive response in a confirm message box.

### Attribute: `multiline`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Applies for prompt action only. Signify whether prompt is textarea(multiliner) or oneliner(textField)Default=false

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the messageBox

### Attribute: `bodyStyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A CSS style specification for the body of the message box. As a general rule, use this attribute to set color and font styles

### Attribute: `buttonType`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies to the control type - confirm
The buttons to display on the message box

### Attribute: `icon`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the following CSS classes
error: Provides the error icon. You can use this icon when displaying error messages.
info: Provides the info icon. You can use this icon when displaying any information.
question: Provides the question icon. You can use this icon in a confirmation message box that prompts a user response.
warning: Provides the warning icon. You can use this icon when displaying a warning message

### Attribute: `modal`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value that specifies if the message box should be a modal window

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Width of the message box in pixels.

### Attribute: `x`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The X (horizontal) coordinate of the upper-left corner of the message box .
ColdFusion ignores this attribute if you do not set the y attribute.

### Attribute: `y`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The Y (vertical) coordinate of the upper-left corner of the message box.
ColdFusion ignores this attribute if you do not set the x attribute.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

