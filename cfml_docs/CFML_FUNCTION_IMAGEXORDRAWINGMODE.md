# Function Name: `ImageXORDrawingMode`

## Description
Sets the paint mode of the image to alternate between the image's current color and the new specified color.

## Return Type
`void`

## Syntax
```cfml
imageXORDrawingMode(image, color)
```

## Arguments

### Argument: `image`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `color`
- **Type**: `string`
- **Required**: Required
- **Default Value**: `black`
- **Description**: XOR alternation color. The values can be:
- Hexadecimal value of the RGB color. For example, specify the color white as `##FFFFFF` or `FFFFFF`.
- String value of color (for example, `black`, `red`, `green`).
- List of three numbers for (R,G,B) values. Each value must be in the range 0-255.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

