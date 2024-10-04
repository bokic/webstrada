# Function Name: `ImageGetIPTCTag`

## Description
 Retrieves the value of the IPTC tag for a ColdFusion image.

## Return Type
`string`

## Syntax
```cfml
imageGetIPTCtag(name, tagName)
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
- **Description**: The IPTC tag name whose value is returned.

## Limitations and Other Info

- **Related Functions**: `cfimage`, `ImageGetBlob`, `ImageGetBufferedImage`, `ImageGetEXIFMetadata`, `ImageGetEXIFTag`, `ImageGetHeight`, `ImageGetIPTCMetadata`, `ImageGetWidth`, `ImageInfo`, `IsImage`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

