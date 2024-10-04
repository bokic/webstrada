# Function Name: `ImageOverlay`

## Description
 Reads two source ColdFusion images and overlays the second source image on the first source image.

## Return Type
`void`

## Syntax
```cfml
imageOverlay(source1, source2 [, rule, alpha])
```

## Arguments

### Argument: `source1`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image that is the bottom layer in the image.

### Argument: `source2`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The image that is the top layer (overlaid on the source1 image) in the image.

### Argument: `rule`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ Compositing Rule

### Argument: `alpha`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ The percentage value of transparency

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-image` module.

