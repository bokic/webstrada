# Function Name: `ImageRotateDrawingAxis`

## Description
 Rotates all subsequent drawing on a ColdFusion image at a specified point by a specified angle.

## Return Type
`void`

## Syntax
```cfml
imageRotateDrawingAxis(name, angle [, x] [, y])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The ColdFusion image on which this operation is performed.

### Argument: `angle`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The rotation angle in degrees.

### Argument: `x`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: The x coordinate for the point of rotation.

### Argument: `y`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: The y coordinate for the point of rotation.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

