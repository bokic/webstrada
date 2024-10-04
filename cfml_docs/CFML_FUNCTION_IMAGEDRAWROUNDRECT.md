# Function Name: `ImageDrawRoundRect`

## Description
 Draws a rectangle with rounded corners.

## Return Type
`void`

## Syntax
```cfml
imageDrawRoundRect(name, x, y, width, height, arcwidth, archeight [, filled])
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

### Argument: `arcwidth`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The horizontal diameter of the arc at the four corners.

### Argument: `archeight`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The vertical diameter of the arc at the four corners.

### Argument: `filled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Specify whether the rectangle is filled

## Limitations and Other Info

- **Related Functions**: `imageDrawBeveledRect`, `imageDrawRect`, `imageClearRect`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

