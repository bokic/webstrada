# Function Name: `EncodeForLDAP`

## Description
Encodes the input string for safe output in LDAP queries to prevent Cross Site Scripting attacks.

## Return Type
`string`

## Syntax
```cfml
encodeForLDAP(string [,canonicalize]);
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
- **Description**: If set to true, canonicalization happens before encoding. If set to false, the given input string will just be encoded. 
When this parameter is not specified, canonicalization will not happen. By default, when canonicalization is performed, both mixed and multiple encodings will be allowed. 
To use any other combinations you should canonicalize using canonicalize method and then do encoding.

## Limitations and Other Info

- **Related Functions**: `encodeForDN`
- **Coldfusion Support**: Minimum version: `11`. Notes: Works on CF11+ but not documented.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-esapi` module.

