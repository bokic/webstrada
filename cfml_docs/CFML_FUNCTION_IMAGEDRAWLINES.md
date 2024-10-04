# Function Name: `ImageDrawLines`

## Description
Draws a sequence of connected lines defined by arrays of x and y coordinates.

## Return Type
`void`

## Syntax
```cfml
imageDrawLines(name, xcords, ycords [, isPolygon] [, filled])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `xcords`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A array of x coordinates.

### Argument: `ycords`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A array of y coordinates.

### Argument: `isPolygon`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Specify whether the lines form a polygon

### Argument: `filled`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Specify whether the polygon is filled

## Limitations and Other Info

- **Related Functions**: `imageDrawLine`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

