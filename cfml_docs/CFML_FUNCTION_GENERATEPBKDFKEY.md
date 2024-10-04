# Function Name: `GeneratePBKDFKey`

## Description
CFML implementation of Password-Based Key-Derivation Function (PBKDF)

## Return Type
`string`

## Syntax
```cfml
generatePBKDFKey(algorithm, passphrase, salt, iterations, keySize);
```

## Arguments

### Argument: `algorithm`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Hashing algorithm used for generating key

### Argument: `passphrase`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Passphrase used for the key. KEEP THIS SECRET.

### Argument: `salt`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string which will be added to the passphrase before encryption.
 The standard recommends a salt length of at least 64 bits (8 characters). The salt needs to be generated using a pseudo-random number generator (e.g. SHA1PRNG)

### Argument: `iterations`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of PBKDEF iterations to perform. A minimum recommended value is 1000

### Argument: `keySize`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The length in bits of the key to generate

## Limitations and Other Info

- **Related Functions**: `hash`, `encrypt`, `decrypt`, `generateSecretKey`
- **Coldfusion Support**: Minimum version: `11`. Notes: Adobe ColdFusion Enterprise includes a java crypto provider that implements these algorithms. These algorithms are available only in enterprise versions:

PBKDF2WithSHA1
PBKDF2WithSHA224
PBKDF2WithSHA256
PBKDF2WithSHA384
PBKDF2WithSHA512
PBKDF2WithSHA512-224
PBKDF2WithSHA512-256
- **Lucee Support**: Minimum version: `5`. Notes: For Lucee it is up to the provider that you have installed, if using the default java crypto provider it only supports "PBKDF2WithHmacSHA1" on Java 1.7 for example. If you are using Java 8 it supports more algorithms (such as `PBKDF2WithHmacSHA512`) .

`iterations` and `keySize` parameters are optional in Lucee.
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-password-encrypt` module

