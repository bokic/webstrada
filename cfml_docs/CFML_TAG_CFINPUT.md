# Tag Name: `cfinput`

## Description
Used within the cfform tag, to place radio buttons, check boxes, or text boxes on a form. Provides input validation for the specified control type.

## Syntax
```cfml
<cfinput name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name for form input element.

### Attribute: `autosuggest`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies entry completion suggestions to display as the user types into a text input. The user can select a suggestion to complete the text entry. The valid value can be either of the following:
- A string consisting of the suggestion values separated by the delimiter specified by the delimiter attribute.
- A bind expression that gets the suggestion values based on the current input text.

Valid only for cfinput type="text".

### Attribute: `autoSuggestBindDelay`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The minimum time between autosuggest bind expression invocations, in seconds. Use this attribute to limit the number of requests that are sent to the server when a user types.

Valid only for cfinput type="text"

### Attribute: `autoSuggestMinLength`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The minimum number of characters required in the text box before invoking a bind expression to return items for suggestion.

Valid only for cfinput type="text".

### Attribute: `bindAttribute`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the HTML tag attribute whose value is set by the bind attribute. You can only specify attributes in the browser‚ HTML DOM tree, not ColdFusion-specific attributes. Ignored if there is no bind attribute.

Valid only for cfinput type="text".

### Attribute: `bindonload`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value that specifies whether to execute the bind attribute expression when first loading the form. Ignored if there is no bind attribute.

Valid only for cfinput type="text".

### Attribute: `id`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: ID for form input element.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `text`
- **Description**: The input control type to create.

Notes:
- file is not supported in Flash.
- image: clickable button with an image.
- datefield: Flash only; date entry field with an expanding calendar for selecting dates.

### Attribute: `label`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Label to put next to the control on a Flash or XML form. Not used for button, hidden, image, reset, or submit types.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: In HTML or XML format, ColdFusion passes the style attribute to the browser or XML. In Flash format, must be a style specification in CSS format.

### Attribute: `class`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Stylesheet class for form input element.

### Attribute: `required`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: 

### Attribute: `mask`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A mask pattern that controls the character pattern that users can enter, or that the form sends to ColdFusion. In HTML and Flash for type=text use:
- A = [A-Za-z]
- X = [A-Za-z0-9]
- 9 = [0-9]
- ? = Any character
- all other = the literal character

In Flash for type=datefield use:
- D = day; can use 0-2 mask characters.
- M = month; can use 0-4 mask characters.
- Y = year; can use 0, 2, or 4 characters.
- E = day in week; can use 0-4 characters.

### Attribute: `validate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: date: verifies format mm/dd/yy.
eurodate: verifies date format dd/mm/yyyy.
time: verifies time format hh:mm:ss.
float: verifies floating point format.
integer: verifies integer format.
telephone: verifies telephone format ###-###-####. The separator can be a blank. Area code and exchange must begin with digit 1 - 9.
zipcode: verifies, in U.S. formats only, 5- or 9-digit format #####-####. The separator can be a blank.
creditcard: strips blanks and dashes; verifies number using mod10 algorithm. Number must have 13-16 digits.
social_security_number: verifies format ###-##-####. The separator can be a blank.
submitonce (ACF-only): Prevents double form submission. Valid Types: Submit and Image only. Valid Formats: HTML/XML.
regular_expression: matches input against pattern attribute.

### Attribute: `validateat`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `onSubmit`
- **Description**: How to do the validation; one or more of the following: onSubmit, onServer or onBlur. onBlur and onSubmit are identical in Flash forms. For multiple values, use a comma-delimited list. Not supported on Railo/Lucee.

### Attribute: `message`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Message text to display if validation fails

### Attribute: `range`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Minimum and maximum value range, separated by a comma. If type = "text" or "password", this applies only to numeric data.

### Attribute: `maxlength`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum length of text entered, if type=text or password.

### Attribute: `pattern`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript regular expression pattern to validate input. ColdFusion uses this attribute only if you specify regex in the validate attribute. Omit leading and trailing slashes.

### Attribute: `onvalidate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to validate user input. The form object, input object, and input object values are passed to the routine, which should return True if validation succeeds, and False otherwise. If used, the validate attribute is ignored.

### Attribute: `onerror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to execute if validation fails.

### Attribute: `size`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Size of input control. Ignored, if type=radio or checkbox. If specified in a Flash form, ColdFusion sets the control width pixel value to 10 times the specified size and ignores the width attribute.

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: HTML: corresponds to the HTML value attribute. Its use depends on control type.
Flash: optional; specifies text for button type inputs: button, submit, and image.

### Attribute: `bind`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Flash bind expression that populates the field with information from other form fields.

### Attribute: `checked`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Selects a control. No value is required. Applies if type=radio or checkbox.

### Attribute: `disabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Disables user input, making the control read-only.
HTML: Passes the attribute directly to the HTML. To enable the input you need to omit this attribute. It does not respect the attribute-value.
Flash: Disables the input when the attribute is set without attribute-value or when the attribute-value is an positive boolean value.

### Attribute: `src`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies to Flash button, reset, submit, and image types, and the HTML image type. URL of an image to use on the button. Flash does not support GIF images.

### Attribute: `onkeyup`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash) to run when the user releases a keyboard key in the control.

### Attribute: `onkeydown`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash) ActionScript to run when the user presses a keyboard key in the control.

### Attribute: `onmouseup`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash) to run when the user presses a mouse button in the control.

### Attribute: `onmousedown`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash) to run when the user releases a mouse button in the control.

### Attribute: `onchange`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash) to run when the control changes due to user action. In Flash, applies to datefield, password, and text types only.

### Attribute: `onclick`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript (HTML/XML) or ActionScript (Flash) to run when the user clicks the control. In Flash, applies to button, checkbox, image, radio, reset, and submit types only.

### Attribute: `daynames`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `S,M,T,W,Th,F,S`
- **Description**: A comma-delimited list that sets the names of the weekdays displayed in the calendar. Sunday is the first day and the rest of the weekday names follow in the normal order. You can use one-/two-letter, three-letter abbreviation or the full name.

### Attribute: `firstdayofweek`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Integer in the range 0-6 specifying the first day of the week in the calendar, 0 indicates Sunday, 6 indicates Saturday.

### Attribute: `monthnames`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A comma-delimited list of the month names that are displayed at the top of the calendar. You can use three-letter-abbreviation or full month name

### Attribute: `enabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying whether the control is enabled. A disabled control appears in light gray. The inverse of the disabled attribute.

### Attribute: `visible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying whether to show the control. Space that would be occupied by an invisible control is blank.

### Attribute: `tooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: Text to display when the mouse pointer hovers over the control.

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies to most Flash types, HTML image type on some browsers. The width of the control, in pixels. For Flash forms, ColdFusion ignores this attribute if you also specify a size attribute value.

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies to most Flash types, HTML image type on some browsers. The height of the control, in pixels. The displayed height might be less than the specified size.

### Attribute: `passthrough`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: This attribute is deprecated. Passes arbitrary attribute-value pairs to the HTML code that is generated for the tag. You can use either of the following formats:
- passthrough="title=""myTitle"""
- passthrough='title="mytitle"'

### Attribute: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The delimiter to use to separate entries in a static autosuggest list. This attribute is meaningful only if the autosuggest attribute is a string of delimited values.

### Attribute: `maxresultsdisplayed`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The maximum number suggestions to display in the autosuggest list.
Valid only for cfinput type="text".

### Attribute: `onbinderror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a JavaScript function to execute if evaluating a bind expression, including an autosuggest bind expression, results in an error. The function must take two attributes: an HTTP status code and a message.

### Attribute: `showautosuggestloadingicon`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: A Boolean value that specifies whether to display an animated icon when loading an autosuggest value for a text input.

### Attribute: `sourcefortooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The URL of a page to display as a tool tip. The page can include HTML markup to control the format, and the tip can include images.
If you specify this attribute, an animated icon appears with the text "Loading..." while the tip is being loaded.

### Attribute: `typeahead`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value that specifies whether the autosuggest feature should automatically complete a user's entry with the first result in the suggestion list.
Valid only for cfinput type="text".

### Attribute: `matchContains`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: true, match returned "contains" the query string. Default is false so that only results that "start with" the query string are returned.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

