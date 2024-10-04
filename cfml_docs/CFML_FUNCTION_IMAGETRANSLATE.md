# Function Name: `ImageTranslate`

## Description
 Copies an image to a new location on the plane.

## Return Type
`void`

## Syntax
```cfml
imageTranslate(name, xTrans, yTrans [, interpolation])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `xTrans`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Displacement in the x direction.

### Argument: `yTrans`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Displacement in the y direction.

### Argument: `interpolation`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `nearest`
- **Description**: Type of interpolation

## Limitations and Other Info

- **Related Functions**: `ImageSetAntialiasing`, `ImageShear`, `ImageTranslateDrawingAxis`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

