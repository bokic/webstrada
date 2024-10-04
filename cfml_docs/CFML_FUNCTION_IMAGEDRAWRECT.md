# Function Name: `ImageDrawRect`

## Description
Draws a rectangle.

## Return Type
`void`

## Syntax
```cfml
imageDrawRect(name, x, y, width, height [, filled])
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

### Argument: `filled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Specify whether the rectangle is filled

## Limitations and Other Info

- **Related Functions**: `imageDrawBeveledRect`, `imageClearRect`, `imageDrawRoundRect`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

