# Function Name: `ImageDrawText`

## Description
 Draws a text string on a ColdFusion image with the baseline of the first character positioned at (x,y) in the image.

## Return Type
`void`

## Syntax
```cfml
imageDrawText(name, str, x, y, attributecollection)
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `str`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The text to draw.

### Argument: `x`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The x coordinate for the start point of the string.

### Argument: `y`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The y coordinate for the start point of the string.

### Argument: `attributecollection`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A structure used to specify the text characteristics. the following keys are supported: 
font: The name of the font used to draw the text string. If you do not specify the font attribute, the text is drawn in the default system font. 
size: The font size for the text string. The default value is 10 points. 
style: The style to apply to the font ( bold,italic,boldItalic,plain (default) ). 
strikethrough: a boolean that specify whether strikethrough is applied to the text image, default is false. 
underline: a boolean that specify whether underline is applied to the text image, default is false.

## Limitations and Other Info

- **Related Functions**: `ImageDrawArc`, `ImageDrawBeveledRect`, `ImageDrawCubicCurve`, `ImageDrawLine`, `ImageDrawLines`, `ImageDrawOval`, `ImageDrawQuadraticCurve`, `ImageDrawRect`, `ImageDrawRoundRect`, `ImageSetAntialiasing`, `ImageSetDrawingColor`, `ImageTranslateDrawingAxis`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

