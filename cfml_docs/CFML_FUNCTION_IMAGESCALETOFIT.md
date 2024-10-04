# Function Name: `ImageScaleToFit`

## Description
Creates a resized image with the aspect ratio maintained.

## Return Type
`void`

## Syntax
```cfml
imageScaleTofit(name, fitWidth, fitHeight [, interpolation] [, blurFactor])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The ColdFusion image on which this operation is performed.

### Argument: `fitWidth`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The width of the bounding box in pixels. You can specify an integer, or an empty string ('') if the fitHeight is specified.

### Argument: `fitHeight`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The height of the bounding box in pixels. You can specify an integer, or an empty string ('') if the fitWidth is specified.

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

- **Related Functions**: `imageResize`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

