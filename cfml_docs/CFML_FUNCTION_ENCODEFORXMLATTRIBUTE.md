# Function Name: `EncodeForXMLAttribute`

## Description
Encodes a string for safe output within an XML attribute to prevent Cross Site Scripting attacks. Use encodeForXML when outputting a variable inside a XML tag body.

## Return Type
`string`

## Syntax
```cfml
encodeForXMLAttribute(string [,canonicalize]);
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The string to encode.

### Argument: `canonicalize`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: If set to true, canonicalization happens before encoding. If set to false, the given input string will just be encoded. 
When this parameter is not specified, canonicalization will not happen. By default, when canonicalization is performed, both mixed and multiple encodings will be allowed. 
To use any other combinations you should canonicalize using canonicalize method and then do encoding.

## Limitations and Other Info

- **Related Functions**: `encodeForXML`, `canonicalize`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**: Minimum version: `4`.
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-esapi` module.

