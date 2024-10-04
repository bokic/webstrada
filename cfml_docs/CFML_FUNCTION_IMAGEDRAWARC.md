# Function Name: `ImageDrawArc`

## Description
 Draws a circular or elliptical arc.

## Return Type
`void`

## Syntax
```cfml
imageDrawArc(name, x, y, width, height, startAngle, archAngle [, filled])
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
- **Description**: The x coordinate of the upper-left corner of the arc.

### Argument: `y`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The y coordinate of the upper-left corner of the arc.

### Argument: `width`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The width of the arc.

### Argument: `height`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The height of the arc.

### Argument: `startAngle`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The beginning angle in degrees.

### Argument: `archAngle`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The angular extent of the arc, relative to the start angle.

### Argument: `filled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Specify whether the arc is filled

## Limitations and Other Info

- **Related Functions**: `ImageDrawCubicCurve`, `ImageDrawOval`, `ImageDrawQuadraticCurve`, `ImageSetAntialiasing`, `ImageSetDrawingColor`, `ImageSetDrawingStroke`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

