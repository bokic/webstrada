# Function Name: `ImageFlip`

## Description
Flips an image across an axis.

## Return Type
`void`

## Syntax
```cfml
imageFlip(name, transpose)
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `transpose`
- **Type**: `string`
- **Required**: Required
- **Default Value**: `vertical`
- **Description**: Transpose the image.

vertical: Flip an image across an imaginary horizontal line that runs through the center of the image.
horizontal: Flip an image across an imaginary vertical line that runs through the center of the image.
diagonal: Flip an image across its main diagonal that runs from the upper-left to the lower-right corner.
antidiagonal: Flip an image across its main diagonal that runs from the upper-right to the lower-left corner.
Or rotate an image clockwise by 90, 180, or 270 degrees.

## Limitations and Other Info

- **Related Functions**: `ImageBlur`, `ImageClearRec`, `ImageNegative`, `ImageNew`, `ImageOverlay`, `ImagePaste`, `ImageResize`, `ImageRotate`, `ImageScaleToFit`, `ImageSetAntialiasing`, `ImageSharpen`, `ImageShear`, `ImageTranslate`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

