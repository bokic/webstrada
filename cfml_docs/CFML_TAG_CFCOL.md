# Tag Name: `cfcol`

## Description
Defines table column header, width, alignment, and text. Used
 within a cftable tag.

## Syntax
```cfml
<cfcol header="" text="">
```

## Attributes / Variants

### Attribute: `header`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Column header text. To use this attribute, you must also
 use the cftable colHeaders attribute.

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `20`
- **Description**: Column width. If the length of data displayed exceeds this
 value, data is truncated to fit. To avoid this, use an
 HTML table tag.

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `left`
- **Description**: Column alignment

### Attribute: `text`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Double-quotation mark-delimited text; determines what to
 display. Rules: same as for cfoutput sections. You can
 embed hyperlinks, image references, and input controls

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

