# Function Name: `CSRFGenerateToken`

## Description
Generates a random token and stores it in the session to protect against Cross-Site Request Forgery (CSRF) attacks. You can optionally provide a specific key to store in the session, and optionally force the generation of a new token.

## Return Type
`string`

## Syntax
```cfml
csrfGenerateToken( [key] [,forceNew] )
```

## Arguments

### Argument: `key`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A random token is generated based on the key provided. This key is stored in the session.

### Argument: `forceNew`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: If set to true, a new token is generated every time the method is called. If false, and in the case where a token already exists [for the key], the same key is returned.

## Limitations and Other Info

- **Related Functions**: `csrfVerifyToken`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**: Minimum version: `4.5`.
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-csrf` module

