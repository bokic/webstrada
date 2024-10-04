# Tag Name: `cftree`

## Description
Inserts a tree control in a form. Validates user selections.
 Used within a cftree tag block. You can use a CFML query
 to supply data to the tree.

## Syntax
```cfml
<cftree name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name for tree control.

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `applet`
- **Description**: - applet: displays the tree using a Java applet in the
 browser,
 - flash: displays the tree using a Flash control
 - object: returns the tree as a ColdFusion structure with the
 name specified by the name attribute, For details of the
 structure contents, see "object format", below.
 - xml: Generates an XMLrepresentation of the tree.
 In XML format forms, includes the generated XML in the
 form. and puts the XML in a string variable with the name
 specified by the name attribute.
 Default: applet

### Attribute: `required`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: - true: user must select an item in tree control
 - false: they do not
 Default: false

### Attribute: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `\\`
- **Description**: Character to separate elements in form variable path.
 Default: \\

### Attribute: `completepath`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: - true: start the Form.treename.path variable with the root
 of the tree path when cftree is submitted.
 - false: omit the root level from the Form.treename.path
 variable; the value starts with the first child node in the
 tree.
 For the preserveData attribute of cfform to work with the
 tree, you must set this attribute to Yes.
 For tree items populated by a query, if you use the
 cftreeitem queryasroot attribute to specify a root name,
 that value is returned. If you do not specify a root name,
 ColdFusion returns the query name.
 Default: false

### Attribute: `appendkey`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: - true: if you use cftreeitem href attributes, ColdFusion
 appends a CFTREEITEMKEY query string variable with
 the value of the selected tree item to the cfform action URL.
 - false: do not append the tree item value to the URL.
 Default: true

### Attribute: `highlighthref`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: - true: highlights as a link the displayed value for any
 cftreeitem tag that specifies a href attribute.
 - false: disables highlighting.
 Default: true

### Attribute: `onvalidate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript function to validate user input. The form object,
 input object, and input object value are passed to the
 specified routine, which should return True if validation
 succeeds; False, otherwise.

### Attribute: `message`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Message to display if validation fails.

### Attribute: `onerror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript function to execute if validation fails.

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
- **Description**: - true: displays tree control text in italics
 - false: it does not
 Default: false

### Attribute: `bold`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: - true: displays tree control text in bold
 - false: it does not
 Default: false

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `320`
- **Description**: Tree control height, in pixels. If you omit this attribute in
 Flash format, Flash automatically sizes the tree.
 Default: 320 (applet only)

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `200`
- **Description**: Tree control width, in pixels. If you omit this attribute in
 Flash format, Flash automatically sizes the tree.
 Default: 200 (applet only)

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
- **Description**: Alignment of the tree control applet object.

### Attribute: `border`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: - true: display a border around the tree control.
 - false: no border
 Default: true

### Attribute: `hscroll`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: - true: permits horizontal scrolling
 - false: no horizontal scrolling
 Default: true

### Attribute: `vscroll`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: - true: permits vertical scrolling
 - false: no vertical scrolling
 Default: true

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: Must be a style specification in CSS format, with the same
 syntax and contents as used in Macromedia Flex for the
 corresponding Flash element.

### Attribute: `enabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying whether the
 control is enabled. A disabled control appears in light gray.
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
- **Description**: Flash only: Text to display when the mouse pointer
 hovers over the control.

### Attribute: `onchange`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: ActionScript to run when the control changes due to user action.
 If you specify an onChange event handler, the Form scope of
 the ColdFusion action page does not automatically get
 information about selected items. The ActionScript onChange
 event handler must handle all changes and selections.

### Attribute: `onblur`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: ActionScript that runs when the calendar loses focus.
 (Added in 7.0.1)

### Attribute: `onfocus`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: ActionScript that runs when the calendar loses focus.
 (Added in 7.0.1)

### Attribute: `notsupported`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `<b>Browser must support Java to <br>view ColdFusion Java Applets!</b>`
- **Description**: Text to display if a page that contains a Java applet-based
 cfform control is opened by a browser that does not
 support Java or has Java support disabled.

### Attribute: `cache`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: A Boolean value that specifies whether to get new data each time the user expands tree nodes, as follows:
 * yes: fetches a node's child items only once, when the node is first expanded
 * no: fetches child items each time the node is expanded.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

