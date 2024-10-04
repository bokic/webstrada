# Tag Name: `cfmenu`

## Description
Creates a menu or tool bar (a horizontally arranged menu). Any menu item can be the top 
 level of a submenu.

## Syntax
```cfml
<cfmenu>
```

## Attributes / Variants

### Attribute: `bgcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The color of the menu background. You can 
 use any valid HTML color specification. 
 This specification is locally overridden by the 
 menuStyle attribute of this tag and any 
 cfmenuitem tag, but does affect the background 
 of the surrounding color of a submenu whose 
 background is controlled by a childStyle 
 attribute

### Attribute: `childstyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A CSS style specification that applies to the 
 items of the top level menu and all child menu 
 items, including the children of submenus. This 
 attribute lets you use a single style specification 
 for all menu items.

### Attribute: `font`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The font to be use for all child menu items. You 
 can use any valid HTML font-family style 
 attribute.

### Attribute: `fontcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The color of the menu text. You can use any 
 valid HTML color specification.

### Attribute: `fontsize`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The size of the font. Use a numeric value, such 
 as 8, to specify a pixel character size. Use a 
 percentage value, such as 80% to specify a 
 size relative to the default font size. 
 Font sizes over 20 pixels can result in submenu 
 text exceeding the menu boundary.

### Attribute: `menustyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A CSS style specification that applies to the 
 menu, including any parts of the menu that do not 
 have items. If you do not specify style information 
 in the cfmenuitem tags, this attribute controls the 
 style of the top-level items.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the menu.

### Attribute: `selectedfontcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The color of the text for the menu item that has 
 the focus. You can use any valid HTML color 
 specification.

### Attribute: `selecteditemcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The color that highlights the menu item that has 
 the focus. You can use any valid HTML color 
 specification.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `horizontal`
- **Description**: The orientation of menu.

### Attribute: `width`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The width of a vertical menu; not valid for 
 horizontal menus.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

