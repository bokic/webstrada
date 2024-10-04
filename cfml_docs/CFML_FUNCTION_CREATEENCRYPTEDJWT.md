# Function Name: `CreateEncryptedJWT`

## Description
Create an encrypted JWT (JSON Web Encryption - JWE)

## Return Type
`string`

## Syntax
```cfml
createEncryptedJWT(payload, encryptOptions, config)
```

## Arguments

### Argument: `payload`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The payload as a string or struct.

### Argument: `encryptOptions`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Encrypt using the key information from given struct

### Argument: `config`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A struct with the following values:
- algorithm : algorithm used for signing.
- encryption : algorithm used for encrypting the payload.
- generateIssuedAt : boolean to indicate whether to generate "iat" field
- generateJti : boolean to indicate whether to generate "jti" field

## Limitations and Other Info

- **Related Functions**: `verifyEncryptedJWT`, `createSignedJWT`
- **Coldfusion Support**: Minimum version: `2023`.

