# Function Name: `ImageShear`

## Description
 Shears an image either horizontally or vertically.

## Return Type
`void`

## Syntax
```cfml
imageShear(name, shear [, direction] [, interpolation])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `shear`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Shear value. Coordinates can be integers or real numbers.

### Argument: `direction`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `horizontal`
- **Description**: Shear direction

### Argument: `interpolation`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `nearest`
- **Description**: Type of interpolation

## Limitations and Other Info

- **Related Functions**: `ImageSetAntialiasing`, `ImageShearDrawingAxis`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

