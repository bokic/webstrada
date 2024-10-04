# Function Name: `BinaryEncode`

## Description
Converts binary data to a string.

## Return Type
`string`

## Syntax
```cfml
binaryEncode(binaryData, encoding)
```

## Arguments

### Argument: `binaryData`
- **Type**: `binary`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string containing encoded binary data.

### Argument: `encoding`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string specifying the encoding method to use to represent
 the data; one of the following:
 - hex: use characters 0-9 and A-F represent the hexadecimal value
 of each byte; for example, 3A.
 - UU: use the UNIX UUencode algorithm to convert the data.
 - base64: use the Base64 algorithm to convert the data.
 - base64URL: modification of the main Base64 standard, which uses the encoding result as filename or URL address.

## Limitations and Other Info

- **Related Functions**: `binaryDecode`
- **Coldfusion Support**: Minimum version: `7`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

