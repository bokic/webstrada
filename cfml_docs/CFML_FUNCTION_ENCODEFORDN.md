# Function Name: `EncodeForDN`

## Description
Encodes the given string for safe output in LDAP Distinguished Names (DN). Intended to prevent LDAP Injection.

## Return Type
`string`

## Syntax
```cfml
encodeForDN( string [,canonicalize]);
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String to encode.

### Argument: `canonicalize`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: If set to true, canonicalization happens before encoding. If set to false, the given input string will just be encoded. The default value for canonicalize is false. 
When this parameter is not specified, canonicalization will not happen. By default, when canonicalization is performed, both mixed and multiple encodings will be allowed. 
To use any other combinations you should canonicalize using canonicalize method and then do encoding.

## Limitations and Other Info

- **Related Functions**: `encodeForLDAP`, `encodeForHTML`, `EncodeForHTMLAttribute`, `EncodeForURL`, `Canonicalize`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-esapi` module.

