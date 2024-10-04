# Function Name: `ImageDrawBeveledRect`

## Description
Draws a rectangle with beveled edges.

## Return Type
`void`

## Syntax
```cfml
imageDrawBeveledRect(name, x, y, width, height, raised [, filled])
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
- **Description**: The x coordinate of the rectangle.

### Argument: `y`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The y coordinate of the rectangle.

### Argument: `width`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The width of the rectangle.

### Argument: `height`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The height of the rectangle.

### Argument: `raised`
- **Type**: `boolean`
- **Required**: Required
- **Default Value**: `false`
- **Description**: Specify whether the rectangle appears raised above the surface or sunk into the surface

### Argument: `filled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Specify whether the rectangle is filled.

## Limitations and Other Info

- **Related Functions**: `imageClearRect`, `imageDrawRect`, `imageDrawRoundRect`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

