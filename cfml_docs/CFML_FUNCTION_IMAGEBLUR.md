# Function Name: `ImageBlur`

## Description
 Smooths (blurs) the ColdFusion image.

## Return Type
`void`

## Syntax
```cfml
imageBlur(name [, blurradius])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `blurradius`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `3`
- **Description**: The size of the blur radius. Value must be greater than or equal to 3 and less than or equal to 10.

## Limitations and Other Info

- **Related Functions**: `ImageSharpen`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

