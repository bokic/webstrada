# Function Name: `CSRFVerifyToken`

## Description
Validates the passed in token against the token stored in the session for a specific key. Used to help prevent Cross-Site Request Forgery (CSRF) attacks.

## Return Type
`boolean`

## Syntax
```cfml
csrfVerifyToken( token [,key] )
```

## Arguments

### Argument: `token`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The passed in token that is to be validated against the token stored in the session. For Adobe Coldfusion, only the first 40 characters of the passed in token are used to verify.

### Argument: `key`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The key against which the token was originally generated.

## Limitations and Other Info

- **Related Functions**: `csrfGenerateToken`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**: Minimum version: `4.5`.
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-csrf` module

