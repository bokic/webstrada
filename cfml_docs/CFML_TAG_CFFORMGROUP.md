# Tag Name: `cfformgroup`

## Description
Creates a container control for multiple form controls.
 Used in the cfform tag body of Macromedia Flash and XML
 forms. Ignored in HTML forms.

## Syntax
```cfml
<cfformgroup type="horizontal">
```

## Attributes / Variants

### Attribute: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: For XML forms can be any XForms group type defined in the XSLT.
 For Flash see the value options and docs for more information.

### Attribute: `query`
- **Type**: `query`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The query to use with the repeater. Flash creates an
 instance of each of the cfformgroup tag's child tags for
 each row in the query. You can use the bind attribute in
 the child tags to use data from the query row for the
 instance.

### Attribute: `startrow`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Used only for the repeater type; ignored otherwise.
 Specifies the row number of the first row of the query to
 use in the Flash form repeater. This attribute is zerobased:
 the first row is row 0, not row 1 (as in most ColdFusion tags).
 Default: 0

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Used only for for the repeater type; ignored otherwise.
 Specifies the maximum number of query rows to use in
 the Flash form repeater. If the query has more rows than
 the sum of the startrow attribute and this value, the
 repeater does not use the remaining rows.

### Attribute: `label`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Label to apply to the form group. In Flash, does the following:
 - For a page or panel form group, determines the label to
 put on the corresponding accordion pleat, the tabnavigator tab,
 or the panel title bar. For a Flash horizontal or vertical form
 group, specifies the label to put to the left of the group.
 - Ignored in Flash for repeater, hbox, hdividedbox, vbox,
 vdividedbox, tile, accordion, and tabnavigator types.

### Attribute: `id`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: ID for form input element.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash: A Flash style specification in CSS format.
 XML: An inline CSS style specification.

### Attribute: `selectedindex`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Used only for accordion and tabnavigator types; ignored
 otherwise. Specifies the page control to display as open,
 where 0 (not 1) specifies the first page control defined in
 the group.

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Width of the group container, in pixels. If you omit this
 attribute, Flash automatically sizes the container width.
 Ignored for Flash repeater type.

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Height of the group container, in pixels. If you omit this
 attribute, Flash automatically sizes the container height.
 Ignored for Flash repeater type.

### Attribute: `enabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying whether the controls in the
 form group are enabled. Disabled controls appear in
 light gray.
 Default: true

### Attribute: `visible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying whether the controls in the
 form group are visible. If the controls are invisible, the
 space that would be occupied by visible controls is blank.
 Default: true

### Attribute: `onchange`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: tabnavigator and accordion types only: ActionScript
 expression or expressions to execute when a new tab or
 accordion page is selected. Note: The onChange event
 occurs when the form first displays.

### Attribute: `tooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: Text to display when the mouse pointer hovers in the
 form group area. If a control in the form group also
 specifies a tooltip, Flash displays the control's tolltip
 when the mouse pointer hovers over the control.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

