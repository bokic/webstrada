# Function Name: `ImageGetEXIFTag`

## Description
 Retrieves the specified EXIF tag in an image.

## Return Type
`string`

## Syntax
```cfml
imageGetEXIFTag(name, tagName)
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `tagName`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The EXIF tag name to be returned.

## Limitations and Other Info

- **Related Functions**: `imageGetEXIFMetadata`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

