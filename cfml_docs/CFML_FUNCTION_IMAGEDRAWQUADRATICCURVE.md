# Function Name: `ImageDrawQuadraticCurve`

## Description
 Draws a curved line. The curve is controlled by a single point.

## Return Type
`void`

## Syntax
```cfml
imageDrawQuadraticCurve(name, ctrlx1, ctrly1, x1, y1, x2, y2)
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `x1`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The x coordinate of the start point of the quadratic curve segment.

### Argument: `y1`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The y coordinate of the start point of the quadratic curve segment.

### Argument: `ctrlx1`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The x coordinate of the first control point of the quadratic curve segment.

### Argument: `ctrly1`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The y coordinate of the first control point of the quadratic curve segment.

### Argument: `x2`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The x coordinate of the end point of the quadratic curve segment.

### Argument: `y2`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The y coordinate of the end point of the quadratic curve segment.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

