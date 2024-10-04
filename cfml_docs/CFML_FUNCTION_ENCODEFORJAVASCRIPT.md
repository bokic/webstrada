# Function Name: `EncodeForJavaScript`

## Description
Encodes the input string for safe output within JavaScript code. The encoding in meant to mitigate Cross Site Scripting (XSS) attacks. This function can provide more protection from XSS than JSStringFormat does.

## Return Type
`string`

## Syntax
```cfml
encodeForJavaScript(string [, canonicalize])
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string to encode.

### Argument: `canonicalize`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: When true runs the canonicalize function against the input before encoding.

### Argument: `canonicalize`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: If set to true, canonicalization happens before encoding. If set to false, the given input string will just be encoded. 
When this parameter is not specified, canonicalization will not happen. By default, when canonicalization is performed, both mixed and multiple encodings will be allowed. 
To use any other combinations you should canonicalize using `canonicalize` method and then do encoding.

## Limitations and Other Info

- **Related Functions**: `jsStringFormat`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**: Minimum version: `4.5`.
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-esapi` module.

