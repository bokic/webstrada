# Function Name: `URLEncodedFormat`

## Description
Generates a URL-encoded string. For example, it replaces spaces
 with %20, and non-alphanumeric characters with equivalent
 hexadecimal escape sequences. Passes arbitrary strings within a
 URL.

## Return Type
`string`

## Syntax
```cfml
urlEncodedFormat(String [, charset])
```

## Arguments

### Argument: `String`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string or a variable that contains one

### Argument: `charset`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The character encoding in which the string is encoded.

## Limitations and Other Info

- **Related Functions**: `encodeForURL`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

