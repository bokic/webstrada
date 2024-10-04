# Tag Name: `cfgrid`

## Description
Used within the cfform tag. Puts a grid control (a table of
 data) in a CFML form. To specify grid columns and row
 data, use the cfgridcolumn and cfgridrow tags, or use the
 query attribute, with or without cfgridcolumn tags.

## Syntax
```cfml
<cfgrid name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of grid element.

### Attribute: `bind`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A bind expression specifying used to fill the 
 contents of the grid. Cannot be used with the 
 query attribute.

### Attribute: `pagesize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The number of rows to display per page for a 
 dynamic grid. If the number of available rows 
 exceeds the page size, the grid displays only 
 the specified number of entries on a single 
 page, and the user navigates between pages 
 to show all data. The grid retrieves data for 
 each page only when it is required for display. 
 This attribute is ignored if you specify a query 
 attribute.

### Attribute: `striperowcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The color to use for one of the alternating 
 stripes. The bgColor setting determines the 
 other color

### Attribute: `preservepageonsort`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Specifies whether to display the page with 
 the current page number, or display page 1, 
 after sorting (or resorting) the grid

### Attribute: `striperows`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Specifies whether to display the page with 
 the current page number, or display page 1, 
 after sorting (or resorting) the grid

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `applet`
- **Description**: - applet: generates a Java applet.
 - Flash: generates a Flash grid control.
 - xml: generates an XMLrepresentation of the grid.
 In XML format forms, includes the generated XML in the form.
 In HTML format forms, puts the XML in a string variable
 with the name specified by the name attribute.
 Default: applet

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `300`
- **Description**: Control's height, in pixels.
 Default for applet: 300

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Control's width, in pixels.
 Default for applet: 300

### Attribute: `autowidth`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: sets column widths so that all columns display within
 grid width.
 No: sets columns to equal widths. User can resize columns.
 Horizontal scroll bars are not available, because if
 you specify a column width and set autoWidth = "Yes",
 CFML sets to this width, if possible

### Attribute: `vspace`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Vertical margin above and below control, in pixels.

### Attribute: `hspace`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Horizontal spacing to left and right of control, in pixels.

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Alignment of the grid cell contents

### Attribute: `query`
- **Type**: `query`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of query associated with grid control.

### Attribute: `insert`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: User can insert row data in grid.
 Takes effect only if selectmode="edit"

### Attribute: `delete`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: User can delete row data from grid.
 Takes effect only if selectmode="edit"

### Attribute: `sort`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: The sort button performs simple text sort on column. User
 can sort columns by clicking column head or by clicking
 sort buttons. Not valid with selectmode=browse.

 Yes: sort buttons display on grid control

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

### Attribute: `textcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `black`
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

### Attribute: `appendkey`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: When used with href, passes CFTREEITEMKEY variable
 with the value of the selected tree item in URL to the
 application page specified in the cfform action
 attribute

### Attribute: `highlighthref`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Yes: Highlights links that are associated with a cftreeitem
 with a URL attribute value.
 No: Disables highlight.

### Attribute: `onvalidate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript function to validate user input. The form object,
 input object, and input object value are passed to the
 specified routine, which should return True if validation
 succeeds; False, otherwise.

### Attribute: `onerror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript function to execute if validation fails.

### Attribute: `griddataalign`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `left`
- **Description**: Left: left-aligns data within column.
 Right: right-aligns data within column.
 Center: center-aligns data within column.

### Attribute: `gridlines`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Yes: enables row and column rules in grid control

### Attribute: `rowheight`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Minimum row height, in pixels, of grid control. Used with
 cfgridcolumn type = "Image"; defines space for graphics to
 display in row.

### Attribute: `rowheaders`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Yes: displays a column of numeric row labels in grid
 control

### Attribute: `rowheaderalign`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `left`
- **Description**: Left: left-aligns data within row header
 Right: right-aligns data within row header
 Center: center-aligns data within row header

### Attribute: `rowheaderfont`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Font of data in column.

### Attribute: `rowheaderfontsize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Size of text in column.

### Attribute: `rowheaderitalic`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: displays grid control text in italics

### Attribute: `rowheaderbold`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: displays grid control text in bold

### Attribute: `rowheadertextcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `black`
- **Description**: Text color for control. For a hex value, use the form:
 textColor = "##xxxxxx", where x = 0-9 or A-F; use two hash
 signs or none.

### Attribute: `colheaders`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Yes: displays a column of numeric row labels in grid
 control

### Attribute: `colheaderalign`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `left`
- **Description**: Left: left-aligns data within row header
 Right: right-aligns data within row header
 Center: center-aligns data within row header

### Attribute: `colheaderfont`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Font of data in column.

### Attribute: `colheaderfontsize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Size of text in column.

### Attribute: `colheaderitalic`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: displays grid control text in italics

### Attribute: `colheaderbold`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: displays grid control text in bold

### Attribute: `colheadertextcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text color for control. For a hex value, use the form:
 textColor = "##xxxxxx", where x = 0-9 or A-F; use two hash
 signs or none.

### Attribute: `bgcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Background color of grid control.

### Attribute: `selectcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Background color for a selected item.

### Attribute: `selectmode`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Selection mode for items in the control.
 - Edit: user can edit grid data. Selecting a cell opens
 the editor for the cell type.
 - Row: user selections automatically extend to the row
 that contains selected cell.
 - Single: user selections are limited to selected cell.
 (Applet only)
 - Column: user selections automatically extend
 to column that contains selected cell. (Applet only)
 - Browse: user can only browse grid data. (Applet only)

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum number of rows to display in grid.

### Attribute: `notsupported`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `<b>Browser must support Java to <br>view ColdFusion Java Applets!</b>`
- **Description**: Text to display if a page that contains a Java applet-based
 cfform control is opened by a browser that does not
 support Java or has Java support disabled.

### Attribute: `picturebar`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: images for Insert, Delete, Sort buttons

### Attribute: `insertbutton`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text for the insert button. Takes effect only if
 selectmode="edit".

### Attribute: `deletebutton`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text of Delete button text. Takes effect only if
 selectmode="edit".

### Attribute: `sortascendingbutton`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Sort button text

### Attribute: `sortdescendingbutton`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Sort button text

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: Must be a style specification in CSS format.
 Ignored for type="text".

### Attribute: `enabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying
 whether the control is enabled. A disabled
 control appears in light gray.
 Default: true

### Attribute: `visible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Flash only: Boolean value specifying
 whether to show the control. Space that would
 be occupied by an invisible control is blank.
 Default: true

### Attribute: `tooltip`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: text to display when the
 mouse pointer hovers over the control.

### Attribute: `onchange`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flash only: ActionScript to run when the control changes
 due to user action in the control.

### Attribute: `bindonload`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: * yes: executes the bind attribute expression when first loading the form.
 * no: does not execute the bind attribute expression until the first bound event.
Ignored if there is no bind attribute.

### Attribute: `selectonload`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: * yes: selects the first row of the grid when the grid loads.
 * no: does not select any rows when the grid loads.

### Attribute: `onblur`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: ActionScript that runs when the grid loses focus.

### Attribute: `onfocus`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: ActionScript that runs when the grid gets focus.

### Attribute: `collapsible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value specifying whether the user can collapse the entire grid by clicking an arrow on the title bar.

### Attribute: `groupfield`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Puts the grid rows into groups, organized by the column specified in this attribute. Each group is collapsible and has a header with the column name, group field value, and number of entries in the group.

### Attribute: `onLoad`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Java Script function that gets called when a grid is loaded for first time

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

