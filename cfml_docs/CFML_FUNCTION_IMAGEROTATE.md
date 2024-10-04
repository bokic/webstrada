# Function Name: `ImageRotate`

## Description
Rotates a ColdFusion image at a specified point by a specified angle.

## Return Type
`void`

## Syntax
```cfml
imageRotate(name [, x] [, y] , angle [, interpolation])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `angle`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The rotation angle in degrees.

### Argument: `x`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `2`
- **Description**: The x coordinate for the point of rotation

### Argument: `y`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `2`
- **Description**: The y coordinate for the point of rotation

### Argument: `interpolation`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `nearest`
- **Description**: Type of interpolation

nearest: Applies the nearest neighbor method of interpolation. Image quality is lower than the other interpolation methods, but processing is fastest.
bilinear: Applies the bilinear method of interpolation. The quality of the image is less pixelated than the default, but processing is slower.
bicubic: Applies the bicubic method of interpolation. Generally, the quality of image is highest with this method and processing is slowest.

## Limitations and Other Info

- **Related Functions**: `imageFlip`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

