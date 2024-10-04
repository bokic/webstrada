# Function Name: `HMac`

## Description
Creates a keyed-hash message authentication code (HMAC), which can be used to verify authenticity and integrity of a message by two parties that share the key.

## Return Type
`string`

## Syntax
```cfml
hmac(message, key [, algorithm] [, encoding] )
```

## Arguments

### Argument: `message`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The message or data to authenticate. This can be a String or a byte array.

### Argument: `key`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The secret key. The key can be a String or byte array.

### Argument: `algorithm`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `HMACMD5`
- **Description**: An algorithm supported by the java crypto provider.

### Argument: `encoding`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `utf-8`
- **Description**: The character encoding to use when converting the message to bytes. Must be a character encoding name recognized by the Java runtime.

## Limitations and Other Info

- **Related Functions**: `hash`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Openbd Support**: Minimum version: `3.1`.
- **Boxlang Support**: Minimum version: `1.0.0`.

