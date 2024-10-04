# Function Name: `EncodeForXML`

## Description
Encodes a string for safe use within an XML tag body. Use `encodeForXMLAttribute` for variables output inside an XML attribute value.

## Return Type
`string`

## Syntax
```cfml
encodeForXML(string , [canonicalize]);
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

- **Related Functions**: `encodeForXMLAttribute`, `canonicalize`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**: Minimum version: `4.0`.
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-esapi` module.

