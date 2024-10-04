# Function Name: `GenerateSecretKey`

## Description
Generates a secure random key value for use in the encrypt and decrypt functions.

## Return Type
`string`

## Syntax
```cfml
generateSecretKey([algorithm] [,keysize])
```

## Arguments

### Argument: `algorithm`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The encryption algorithm used to generate the key. 
NOTE: You cannot use `generateSecretKey()` to create a key for the `CFMX_COMPAT` default algorithm in `encrypt()` and `decrypt()` functions.

### Argument: `keysize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `128`
- **Description**: Number of bits requested in the key for the specified algorithm (when allowed by JDK).

## Limitations and Other Info

- **Related Functions**: `encrypt`, `decrypt`
- **Coldfusion Support**: Minimum version: `7`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

