# Function Name: `ImageSharpen`

## Description
 Sharpens a ColdFusion image by using the unsharp mask filter.

## Return Type
`void`

## Syntax
```cfml
imageSharpen(name [, gain])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `gain`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1.0`
- **Description**: Gain values can be integers or real numbers.

## Limitations and Other Info

- **Related Functions**: `ImageBlur`, `ImageSetAntialiasing`, `IsImageFile`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

