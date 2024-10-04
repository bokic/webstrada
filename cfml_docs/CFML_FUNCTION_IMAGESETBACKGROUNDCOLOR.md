# Function Name: `ImageSetBackgroundColor`

## Description
 Sets the background color for the ColdFusion image. The background color is used for clearing a region. Setting the background color only affects the subsequent imageClearRect calls

## Return Type
`void`

## Syntax
```cfml
imageSetBackgroundColor(name, color)
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The ColdFusion image on which this operation is performed.

### Argument: `color`
- **Type**: `string`
- **Required**: Required
- **Default Value**: `black`
- **Description**: Background color

## Limitations and Other Info

- **Related Functions**: `ImageClearRect`, `ImageSetAntialiasing`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

