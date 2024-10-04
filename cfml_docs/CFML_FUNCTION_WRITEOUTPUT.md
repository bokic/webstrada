# Function Name: `WriteOutput`

## Description
 Appends text to the page-output stream.
 This function writes to the page-output stream regardless of
 conditions established by the cfsetting tag.

## Return Type
`boolean`

## Syntax
```cfml
writeOutput(string)
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string, or a variable that contains one

### Argument: `encodeFor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF2016+ Lucee5.1.0.8+ Wraps the result with an encodeFor function.

## Limitations and Other Info

- **Related Functions**: `cfoutput`, `encodeForHTML`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

