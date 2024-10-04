# Function Name: `ImageWriteBase64`

## Description
 Writes Base64 images to the specified filename and destination.

## Return Type
`void`

## Syntax
```cfml
imageWriteBase64(name, destination, format [, inHTMLFormat] [, overwrite])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `destination`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The path for the destination file.

### Argument: `format`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: the image format

### Argument: `inHTMLFormat`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specify whether Base64 output includes the headers used by the Base64 images in the HTML 'img' tag ('data:image/{format};base64,...')

### Argument: `overwrite`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: If set to true, overwrites the destination file.

## Limitations and Other Info

- **Related Functions**: `cfimage`, `ImageReadBase64`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

