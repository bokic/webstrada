# Function Name: `ImageAddBorder`

## Description
 Adds a rectangular border around the outside edge of a ColdFusion image.

## Return Type
`void`

## Syntax
```cfml
imageAddBorder(name, thickness [, color] [, bordertype])
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image on which this operation is performed.

### Argument: `thickness`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Thickness of the border in pixels. The default value is 1. The border is added to the outside edge of the image; the image area is increased accordingly.

### Argument: `color`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `black`
- **Description**: Only valid if the borderType is not specified or if borderType = 'constant'.

### Argument: `bordertype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `constant`
- **Description**: The type of border.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

