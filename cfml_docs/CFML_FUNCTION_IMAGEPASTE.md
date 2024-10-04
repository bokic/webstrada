# Function Name: `ImagePaste`

## Description
 Takes two images and an (x,y) coordinate and draws the second image over the first image with the upper-left corner at coordinate (x,y).

## Return Type
`void`

## Syntax
```cfml
imagePaste(image1, image2, x, y)
```

## Arguments

### Argument: `image1`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The bottom image.

### Argument: `image2`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image that is pasted on top of image1.

### Argument: `x`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The x coordinate where the upper-left corner of image2 is pasted.

### Argument: `y`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The y coordinate where the upper-left corner of image2 is pasted.

## Limitations and Other Info

- **Related Functions**: `ImageCopy`, `ImageOverlay`, `ImageSetAntialiasing`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

