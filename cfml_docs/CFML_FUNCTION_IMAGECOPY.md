# Function Name: `ImageCopy`

## Description
 Copies a rectangular area of an image.

## Return Type
`any`

## Syntax
```cfml
imageCopy(name, x, y, width, height [, dx] [, dy])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The ColdFusion image on which this operation is performed.

### Argument: `x`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The x coordinate of the source rectangle.

### Argument: `y`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The y coordinate of the source rectangle.

### Argument: `width`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The width of the source rectangle.

### Argument: `height`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The height of the source rectangle.

### Argument: `dx`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The x coordinate of the destination rectangle.

### Argument: `dy`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The y coordinate of the destination rectangle.

## Limitations and Other Info

- **Related Functions**: `ImageNew`, `ImagePaste`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

