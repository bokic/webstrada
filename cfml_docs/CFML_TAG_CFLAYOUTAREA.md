# Tag Name: `cflayoutarea`

## Description
Defines a region within a cflayout tag body, such as an 
 individual tab of a tabbed layout. This tag is not used in 
 Flash forms.

## Syntax
```cfml
<cflayoutarea>
```

## Attributes / Variants

### Attribute: `position`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `top`
- **Description**: The position...(docs don't explain this one).

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `center`
- **Description**: Specifies how to align child controls within the 
 layout area.

### Attribute: `closable`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value specifying whether the area can close. 
 Specifying this attribute adds an x icon on the tab or 
 title bar that a user can click to close the area. 
 You cannot use this attribute for border layout areas 
 with a position attribute value of center.

### Attribute: `collapsible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value specifying whether the area can collapse.
 Specifying this attribute adds a >> or << icon on the 
 title bar that a user can click to collapse the area. 
 You cannot use this attribute for border layout areas 
 with a position attribute value of center.

### Attribute: `disabled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value specifying whether the tab is disabled, 
 that is, whether user can select the tab to display its 
 contents. Disabled tabs are greyed out. Ignored if the 
 selected attribute value is true.

### Attribute: `initCollapsed`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value specifying whether the area is initially 
 collapsed. You cannot use this attribute for border layout 
 areas with a position attribute value of center. Ignored 
 if the collapsible attribute value is false.

### Attribute: `initHide`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value specifying whether the area is initially 
 hidden. To show an initially hidden area, use the 
 ColdFusion.Layout.showArea or ColdFusion.Layout.showTab 
 function. You cannot use this attribute for border layout 
 areas with a position attribute value of center.

### Attribute: `maxSize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For layouts with top or bottom position attributes, the maximum 
 height of the area, in pixels, that you can set by dragging a
 splitter. For layouts with left or right position attributes,
 the maximum width of the area. You cannot use this attribute
 for border layout areas with a position attribute value of 
 center.

### Attribute: `minSize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For layouts with top or bottom position attributes, the minimum
 height of the area, in pixels, that you can set by dragging a 
 splitter. For layouts with left or right position attributes, 
 the minimum width of the area., You cannot use this attribute 
 for border layout areas with a position attribute value of center.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the layout area.

### Attribute: `onBindError`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a JavaScript function to execute if evaluating a 
 bind expression results in an error. The function must take 
 two attributes: an HTTP status code and a message. If you omit 
 this attribute, and have specified a global error handler 
 (by using the ColdFusion.setGlobalErrorHandlerfunction ), 
 it displays the error message; otherwise a default error 
 pop-up displays.

### Attribute: `overflow`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `auto`
- **Description**: Specifies how to display child content whose size would cause 
 the control to overflow the window boundaries. Ã”Ã¸Î© In Internet Explorer, layout areas with
 the visible setting expand to fit the size of the contents, 
 rather than having the contents extend beyond the layout area.

### Attribute: `selected`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value specifying whether this tab is initially 
 selected so that its contents appears in the layout.

### Attribute: `size`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For hbox layouts and border layouts with top or bottom position
 attributes, the initial height of the area. For vbox layouts 
 and border layouts with left or right position attributes, the
 initial width of the area. For hbox and vbox layouts, you can
 use any valid CSS length or percent format 
 (such as 10, 10% 10px, or 10em) for this attribute. For border
 layouts, this attribute value must be an integer number of 
 pixels. You cannot use this attribute for border layout areas
 with a position attribute value of center. ColdFusion 
 automatically determines the center size based on the 
 size of all other layout areas.

### Attribute: `source`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A URL that returns the layout area contents. ColdFusion uses 
 standard page path resolution rules. You can use a bind expression
 with dependencies in this attribute. If file specified in this 
 attribute includes tags that use AJAX features, such as cfform, 
 cfgrid, and cfpod, you must use the cfajaximport tag on the page
 that includes the cflayoutarea tag. For more information, 
 see cfajaximport.

### Attribute: `splitter`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value specifying whether the layout area has a divider 
 between it and the adjacent layoutarea control. Users can drag the
 splitter to change the relative sizes of the areas. If this 
 attribute is set true on a left or right position layout area, 
 the splitter resizes the area and its adjacent area horizontally.
 If this attribute is set true on a top or bottom position 
 layout area, the splitter resizes the layout vertically. 
 You cannot use this attribute for border layout areas with 
 a position attribute value of center

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A CSS style specification that controls the appearance of the area.

### Attribute: `title`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For tab layouts, the text to display on the tab. For border 
 layouts, if you specify this attribute ColdFusion creates 
 a title bar for the layout area with the specified text as 
 the title. By default, these layouts do not have a title 
 bar if they are not closable or collapsible. You cannot 
 use this attribute for border layout areas with a position 
 attribute value of center.

### Attribute: `refreshonactivate`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: * true: Refresh the contents of the tab by running the source bind expression whenever the tab display region shows (for example, when the user selects the tab), in addition to when bind events occur.
 * false: Refresh the tab display region only when the bind expression is triggered by its bind event.

### Attribute: `titleIcon`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the location of the icon to display with the title.

### Attribute: `bindOnLoad`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value that specifies whether to execute 
 the bind attribute expression when first loading the 
 form. Ignored if there is no bind attribute.

## Limitations

- **Must be nested inside**: `cflayout`
- **Must not be nested inside**: *None*

