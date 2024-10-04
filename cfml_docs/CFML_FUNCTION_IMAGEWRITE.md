# Function Name: `ImageWrite`

## Description
 Writes a ColdFusion image to the specified filename or destination.

## Return Type
`void`

## Syntax
```cfml
imageWrite(name [, destination] [, quality] [, overwrite])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `destination`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The path to write the file.

If you create the image with the ImageNew function or another operation where you do not specify the filename, specify the destination parameter. The file format is derived from the extension of the filename. The default value for this parameter is the filename of the original image source.

### Argument: `quality`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Defines the JPEG quality used to encode the image. This parameter applies only to destination files with an extension of JPG or JPEG. Valid values are fractions that range from 0 through 1 (the lower the number, the lower the quality). The default value is 0.75.

### Argument: `overwrite`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: If set to true, overwrites the destination file

## Limitations and Other Info

- **Related Functions**: `cfimage`, `GetWriteableImageFormats`, `ImageNew`, `ImageRead`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

