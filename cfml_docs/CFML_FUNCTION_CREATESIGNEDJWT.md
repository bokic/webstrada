# Function Name: `CreateSignedJWT`

## Description
Create a signed JWT (JSON Web Signature - JWS)

## Return Type
`string`

## Syntax
```cfml
createSignedJWT(payload, signOptions, config)
```

## Arguments

### Argument: `payload`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The payload as a string or struct.

### Argument: `signOptions`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Create the signature using the key information from the given struct

### Argument: `config`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A struct with the following values:
- algorithm : algorithm used for signing.
- generateIssuedAt : boolean to indicate whether to generate "iat" field
- generateJti : boolean to indicate whether to generate "jti" field

## Limitations and Other Info

- **Related Functions**: `verifySignedJWT`, `createEncryptedJWT`
- **Coldfusion Support**: Minimum version: `2023`.

