# Function Name: `ImageShearDrawingAxis`

## Description
 Shears the drawing canvas.

## Return Type
`void`

## Syntax
```cfml
imageShearDrawingAxis(name, shrx, shry)
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `shrx`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The multiplier by which coordinates are shifted in the positive x axis direction as a function of the y coordinate.e

### Argument: `shry`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The multiplier by which coordinates are shifted in the positive y axis direction as a function of the x coordinate.

## Limitations and Other Info

- **Related Functions**: `ImageRotateDrawingAxis`, `ImageSetAntialiasing`, `ImageShear`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

