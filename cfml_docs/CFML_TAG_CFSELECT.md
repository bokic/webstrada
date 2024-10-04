# Tag Name: `cfselect`

## Description
Constructs a drop-down list box form control. Used within a
 cfform tag.

 You can populate the list from a query, or by using the HTML
 option tag.

## Syntax
```cfml
<cfselect name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the select form element

### Attribute: `id`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: ID for form input element.

### Attribute: `bind`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A bind expression that dynamically sets an attribute 
 of the control.

### Attribute: `bindAttribute`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the HTML tag attribute whose value is set 
 by the bind attribute. You can only specify attributes 
 in the browser‚ HTML DOM tree, not ColdFusion- 
 specific attributes. 
 Ignored if there is no bind attribute.

### Attribute: `bindOnLoad`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value that specifies whether to execute 
 the bind attribute expression when first loading the 
 form. Ignored if there is no bind attribute.

### Attribute: `editable`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Boolean value specifying whether you can edit the 
 contents of the control.

### Attribute: `label`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Label to put next to the control on a Flash or XML-format form.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: In HTML or XML format forms, ColdFusion passes the
 style attribute to the browser or XML.
 In Flash format, must be a style specification in CSS
 format, with the same syntax and contents as used in
 Macromedia Flex for the corresponding Flash element.
 Post alpha we will document specifics.

### Attribute: `sourceForTooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The URL of a page to display as a tool tip. The page 
 can include CFML and HTML markup to control the 
 tip contents and format, and the tip can include 
 images. 
 If you specify this attribute, an animated icon 
 appears with the text "Loading..." while the tip is 
 being loaded.

### Attribute: `size`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: Number of entries to display at one time. The default, 1,
 displays a drop-down list. Any other value displays a list
 box with size number of entries visible at one time.

### Attribute: `required`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: If true a list element must be selected when form is submitted.
 Note: This attribute has no effect if you omit the size
 attribute or set it to 1 because the browser always submits
 the displayed item. You can work around this issue format
 forms by having an initial option tag with value=" " (note the
 space character between the quotation marks).
 Default: false

### Attribute: `message`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Message to display if required="true" and no selection is made.

### Attribute: `onerror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to execute if validation fails.

### Attribute: `multiple`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: - true: allow selecting multiple elements in drop-down list
 - false: don't allow selecting multiple elements
 Default: false

### Attribute: `query`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of query to populate drop-down list.

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Query column to use for the value of each list element.
 Used with query attribute.

### Attribute: `display`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Query column to use for the display label of each list
 element. Used with query attribute.

### Attribute: `group`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Query column to use to group the items in the drop-down
 list into a two-level hierarchical list.

### Attribute: `queryposition`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `above`
- **Description**: If you populate the options list with a query and use HTML
 option child tags to specify additional entries, determines
 the location of the items from the query relative to the items
 from the option tags:
 - above: Put the query items above the options items.
 - below: Put the query items below the options items.
 Default: above

### Attribute: `selected`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: One or more option values to preselect in the selection list.
 To specify multiple values, use a comma-delimited list. This
 attribute applies only if selection list items are generated
 from a query. The cfform preservedata attribute value can
 override this value.

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
 ActionScript to run when the user depresses a keyboard
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
- **Description**: JavaScript to run when the user clicks the control.

### Attribute: `enabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying whether to show the control.
 Space that would be occupied by an invisible control is
 blank.
 Default: true

### Attribute: `visible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying whether to show the control.
 Space that would be occupied by an invisible control is
 blank.
 Default: true

### Attribute: `tooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: Text to display when the mouse pointer hovers over the control.

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The height of the control, in pixels.

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The width of the control, in pixels.

### Attribute: `passthrough`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: This attribute is deprecated.
 
 Passes arbitrary attribute-value pairs to the HTML code
 that is generated for the tag. You can use either of the
 following formats:
 
 passthrough="title=""myTitle"""
 passthrough='title="mytitle"'

### Attribute: `onbinderror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a JavaScript function to execute if evaluating a bind expression results in an error. The function must take two attributes: an HTTP status code and a message.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

