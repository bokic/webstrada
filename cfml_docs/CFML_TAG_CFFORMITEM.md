# Tag Name: `cfformitem`

## Description
Inserts a horizontal line, a vertical line, a spacer,
 or text in a Flash form. Used in the cfform or cfformgroup
 tag body for Flash and XML forms. Ignored in HTML forms.

## Syntax
```cfml
<cfformitem type="html">
```

## Attributes / Variants

### Attribute: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Form item type. See docs for more details.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash: Must be a style specification in CSS format.
 Ignored if the type attribute is HTML or text.
 XML: ColdFusion passes the style attribute to the XML.
 ColdFusion skins include the style attribute to the
 generated HTML.

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Width of the item, in pixels. If you omit this attribute, Flash
 automatically sizes the width. In ColdFusion XSL skins,
 use the style attribute, instead.

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Height of the item, in pixels. If you omit this attribute,
 Flash automatically sizes the width. In ColdFusion XSL
 skins, use the style attribute, instead.

### Attribute: `enabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Boolean value specifying whether the control is enabled.
 Disabled text appear in light gray. Has no effect on
 spacers and rules.
 Default: true

### Attribute: `visible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Boolean value specifying whether to show the control.
 Space that would be occupied by an invisible control is
 blank. Has no effect on spacers.
 Default: true

### Attribute: `tooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text to display when the mouse pointer hovers over the
 control. Has no effect on spacers.

### Attribute: `bind`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Flash bind expression that populates the field with
 information from other form fields. If you use this
 attribute, ColdFusion MX ignores any text that you
 specify in the body of the cftextitem tag. This attribute
 can be useful if the cfformitem tag is in a cfformgroup
 type="repeater" tag.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

