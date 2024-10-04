# Tag Name: `cfgridcolumn`

## Description
Used with the cfgrid tag in a cfform. Use this tag to specify
 column data in a cfgrid control. The font and alignment
 attributes used in cfgridcolumn override global font or
 alignment settings defined in cfgrid.

## Syntax
```cfml
<cfgridcolumn name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of grid column element. If grid uses a query, column
 name must specify name of a query column.

### Attribute: `header`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Column header text. Used only if cfgrid colHeaders = "Yes".

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Column width, in pixels.

### Attribute: `font`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `arial`
- **Description**: Font of data in column.

### Attribute: `fontsize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Size of text in column.

### Attribute: `italic`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: displays grid control text in italics

### Attribute: `bold`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: displays grid control text in bold

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

### Attribute: `href`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: URL or query column name that contains a URL to hyperlink
 each grid column with.

### Attribute: `hrefkey`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The query column to use for the value appended to the href
 URL of each column, instead of the column's value.

### Attribute: `target`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Frame in which to open link specified in href.

### Attribute: `select`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Yes: user can select the column in grid control.

### Attribute: `display`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: No: hides column

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: image: grid displays image that corresponds to value in
 column (a built-in CFML image name, or an image in
 cfide\classes directory or subdirectory referenced with
 relative URL). If image is larger than column cell, it is
 clipped to fit. Built-in image names

### Attribute: `headerfont`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Font of data in column.

### Attribute: `headerfontsize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Size of text in column.

### Attribute: `headeritalic`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Yes: displays grid control text in italics

### Attribute: `headerbold`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Yes: displays grid control text in bold

### Attribute: `headertextcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text color for control. For a hex value, use the form:
 textColor = "##xxxxxx", where x = 0-9 or A-F; use two hash
 signs or none.

### Attribute: `dataalign`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Column data alignment

### Attribute: `headeralign`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Column header text alignment

### Attribute: `numberformat`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Format for displaying numeric data in grid. See
 numberFormat mask characters.

### Attribute: `values`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Formats cells in column as drop-down list boxes; specify
 items in drop-down list. Example:
 values = "arthur, scott, charles, 1-20, mabel"

### Attribute: `valuesdisplay`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maps elements in values attribute to string to display in
 drop-down list. Delimited strings and/or numeric range(s).

### Attribute: `valuesdelimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maps elements in values attribute to string to display in
 drop-down list. Delimited strings and/or numeric range(s).

### Attribute: `mask`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A mask pattern that controls the character pattern
 that the form displays or allows users to input and
 sends to ColdFusion.
 For currency type data, use currency symbol.
 For text or numeric type data use:
 - A = [A-Za-z]
 - X = [A-Za-z0-9]
 - 9 = [0-9]
 - ? = Any character
 - all other = the literal character
 For date type data use `Ext.Date` masks.

### Attribute: `headerIcon`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Header Icon for grid column

## Limitations

- **Must be nested inside**: `cfgrid`
- **Must not be nested inside**: *None*

