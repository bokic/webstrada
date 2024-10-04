# Function Name: `ImageTranslateDrawingAxis`

## Description
 Translates the origin of the image context to the point (x,y) in the current coordinate system. Modifies the image context so that its new origin corresponds to the point (x,y) in the image's original coordinate system.

## Return Type
`void`

## Syntax
```cfml
imageTranslateDrawingAxis(name, x, y)
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `x`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: x coordinate

### Argument: `y`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: y coordinate

## Limitations and Other Info

- **Related Functions**: `ImageSetAntialiasing`, `ImageSetDrawingColor`, `ImageSetDrawingStroke`, `ImageSetDrawingTransparency`, `ImageShearDrawingAxis`, `ImageTranslate`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

