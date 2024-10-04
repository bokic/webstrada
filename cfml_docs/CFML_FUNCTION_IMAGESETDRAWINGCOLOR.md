# Function Name: `ImageSetDrawingColor`

## Description
 Sets the current drawing color for ColdFusion images. All subsequent graphics operations use the specified color.

## Return Type
`void`

## Syntax
```cfml
imageSetDrawingColor(name, color)
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The ColdFusion image on which this operation is performed.

### Argument: `color`
- **Type**: `string`
- **Required**: Required
- **Default Value**: `black`
- **Description**: Color

## Limitations and Other Info

- **Related Functions**: `ImageSetAntialiasing`, `ImageSetBackgroundColor`, `ImageSetDrawingStroke`, `ImageSetDrawingTransparency`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

