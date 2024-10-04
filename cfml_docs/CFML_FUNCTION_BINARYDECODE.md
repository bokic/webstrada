# Function Name: `BinaryDecode`

## Description
Converts a string to a binary object. Used to convert
 binary data that has been encoded into string format
 back into binary data.

## Return Type
`binary`

## Syntax
```cfml
binaryDecode(string, encoding)
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string containing encoded binary data.

### Argument: `encoding`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string specifying the algorithm used to encode the original
 binary data into a string; must be one of the following:
 - hex: characters 0-9 and A-F represent the hexadecimal value
 of each byte; for example, 3A.
 - UU: data is encoded using the UNIX UUencode algorithm.
 - base64: data is encoded using the Base64 algorithm.
 - base64URL: modification of the main Base64 standard, which uses the encoding result as filename or URL address.

## Limitations and Other Info

- **Related Functions**: `binaryEncode`
- **Coldfusion Support**: Minimum version: `7`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

