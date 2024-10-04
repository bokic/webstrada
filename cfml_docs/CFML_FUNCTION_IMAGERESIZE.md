# Function Name: `ImageResize`

## Description
 Resizes a ColdFusion image.

## Return Type
`void`

## Syntax
```cfml
imageResize(name, width, height, interpolation, blurfactor)
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `width`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: New width of the image. If this value is blank, the width is calculated proportionately to the height.

### Argument: `height`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: New height of the image. If this value is blank, the height is calculated proportionately to the width.

### Argument: `interpolation`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The interpolation method for resampling. You can specify a specific interpolation algorithm by name (for example, hamming), by image quality (for example, mediumQuality), or by performance (for example, highestPerformance). 

### Argument: `blurfactor`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The blur factor used for resampling. The higher the blur factor, the more blurred the image (also, the longer it takes to resize the image).

## Limitations and Other Info

- **Related Functions**: `imageCrop`, `imageScaleTofit`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

