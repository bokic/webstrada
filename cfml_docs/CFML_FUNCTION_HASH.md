# Function Name: `Hash`

## Description
Converts a string into a fixed length hexadecimal string.

NOTE: The result is useful for comparison and validation, such as storing and validating a hashed password without exposing the original password.

## Return Type
`string`

## Syntax
```cfml
hash(string [, algorithm [, encoding]] [, additionalIterations])
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `algorithm`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `MD5`
- **Description**: CF7+ A supported algorithm such as MD5,SHA,SHA-256,SHA-384, or SHA-512. Of those listed SHA-512 is the strongest and generates a 128 character hex result.

NOTE: The Enterprise Edition of ColdFusion installs the RSA BSafe Crypto-J library, which provides FIPS-140 Compliant Strong Cryptography. This includes additional algorithms. You can also install additional cryptography algorithms and use those hashing algorithms.

### Argument: `encoding`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `UTF-8`
- **Description**: CF7+ A string specifying the encoding to use when converting the string to byte data used by the hash algorithm.
Must be a character encoding name recognized by the Java runtime.

NOTE: The default is specified by the value of `defaultCharset` in the `neo-runtime.xml` file, which is normally `UTF-8`. 
NOTE: This is ignored when using the `CFMX_COMPAT` algorithm.

### Argument: `additionalIterations`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: CF10+ Iterates the number of times the hash is computed to create a more computationally intensive hash. Lucee and Adobe CF implement this differently (off by one), see compatibility notes below.

NOTE: This parameter appears to be ignored if the `CFMX_COMPAT` default algorithm is used.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `4.5`. Notes: CF7 added additional algorithm support, CF10 added the iterations option. CF2018 uses `additionalIterations` as the iterations param.
- **Lucee Support**: Notes: The iterations value represents the total number of hashes on Lucee, in Adobe CF the value is the number of additional iterations. In Lucee5+, the iterations param is `numIterations`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

