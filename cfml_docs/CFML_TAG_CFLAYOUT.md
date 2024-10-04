# Tag Name: `cflayout`

## Description
Creates a region of its container (such as the browser 
 window or a cflayoutarea tag) with a specific layout 
 behavior: a bordered area, a horizontal or vertically 
 arranged box, or a tabbed navigator.

## Syntax
```cfml
<cflayout type="accordion">
```

## Attributes / Variants

### Attribute: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: `border`
- **Description**: The type of layout.

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `center`
- **Description**: Specifies the default alignment of the content of 
 child layout areas. Each cflayoutarea tag can specify 
 an alignment attribute to override this value.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the layout region. Must be unique 
 on a page.

### Attribute: `padding`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies only to hbox and vbox layouts.
 You can use any valid CSS length or percent format, 
 such as 10, 10% 10px, or 10em, for this attribute. 
 The padding is included in the child layout area 
 and takes the style of the layout area.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A CSS style specification that defines layout styles.

### Attribute: `tabheight`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A CSS style specification that defines layout stApplies only to tab type layouts. Specifies the 
 height of the content area of all child layout 
areas. You can override this setting by 
specifying a height setting in an individual 
cflayoutarea tag style attributes.

### Attribute: `tabposition`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `top`
- **Description**: Applies only to tab type layouts. Specifies the 
 location of the tabs relative to the tab region 
 contents.

### Attribute: `titlecollapse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Specify whether title bar should act as collapse/expand toggle or not; default=true.

### Attribute: `activeontop`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specify whether active tab needs to be on top always; default=false.

### Attribute: `fillheight`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: True to adjust the active item's height to fill the available space in the container; default=true.

### Attribute: `fitToWindow`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: A Boolean value that specifies whether the border layout should occupy 100% of the width and height of the window

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: 

### Attribute: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: 

### Attribute: `buttonStyle`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: 

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

