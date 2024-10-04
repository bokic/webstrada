# Tag Name: `cfmenuitem`

## Description
Defines an entry in a menu, including an item that is the head of a submenu.

## Syntax
```cfml
<cfmenuitem>
```

## Attributes / Variants

### Attribute: `display`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The text to show as the menu item label.

### Attribute: `childstyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A CSS style specification that applies to all child 
 menu items, including the children of submenus.

### Attribute: `divider`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: This attribute specifies that the item is a divider. If 
 you specify this attribute, you cannot specify any 
 other attributes.

### Attribute: `href`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A URL link to activate or JavaScript function to 
 call when the user clicks the menu item.

### Attribute: `image`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: URL of an image to display at the left side of the 
 menu item. The file type can be any format the 
 browser can display. 
 For most displays, you should use 15x15 pixel 
 images, because larger images conflict with the 
 menu item text

### Attribute: `menustyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A CSS style specification that controls the overall 
 style of any submenu of this menu item. This 
 attribute controls the submenu of the current menu 
 item, but not to any child submenus of the submenu.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the menu item.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A CSS style specification that applies to the current 
 menu item only. It is not overridden by the 
 childStyle attribute.

### Attribute: `target`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The target in which to display the contents 
 returned by the href attribute. The attribute can be 
 a browser window or frame name, an HTML target 
 value, such as _self.

## Limitations

- **Must be nested inside**: `cfmenu`
- **Must not be nested inside**: *None*

