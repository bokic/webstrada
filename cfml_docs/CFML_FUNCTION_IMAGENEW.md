# Function Name: `ImageNew`

## Description
 Creates a ColdFusion image.

## Return Type
`any`

## Syntax
```cfml
imageNew([source] [, width] [, height] [, imagetype] [, canvascolor])
```

## Arguments

### Argument: `source`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The source image path, URL, a variable that is read into the new image, or a Java buffered image.

### Argument: `width`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The width of the new image. Valid when the height is specified and the source is not.

### Argument: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The height of the new image. Valid when the width is specified and the source is not.

### Argument: `imagetype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The type of the image to create (Valid only when width and height are specified),

### Argument: `canvascolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `black`
- **Description**: Color of the image canvas. Possible values are:

Hexadecimal value of RGB color. For example, specify the color white as ##FFFFFF or FFFFFF.
String value of color (for example, 'black', 'red', 'green').
List of three numbers for (R,G,B) values. Each value must be in the range 0-255.

## Limitations and Other Info

- **Related Functions**: `cfimage`, `ImageCopy`, `ImageRead`, `ImageReadBase64`, `ImageSetDrawingColor`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

