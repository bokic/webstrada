# Function Name: `Canonicalize`

## Description
Canonicalize or decode the input string. Canonicalization is simply the operation of reducing a possibly encoded string down to its simplest form. This is important because attackers frequently use encoding to change their input in a way that will bypass validation filters, but still be interpreted properly by the target of the attack. Note that data encoded more than once is not something that a normal user would generate and should be regarded as an attack.

## Return Type
`string`

## Syntax
```cfml
canonicalize(input, restrictMultiple, restrictMixed)
```

## Arguments

### Argument: `input`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String to be encoded.

### Argument: `restrictMultiple`
- **Type**: `boolean`
- **Required**: Required
- **Default Value**: *None*
- **Description**: If set to true, multiple encoding is restricted. This argument can be set to true to restrict the input if multiple or nested encoding is detected. If this argument is set to true, and the given input is multiple or nested encoded using one encoding scheme an error will be thrown.

### Argument: `restrictMixed`
- **Type**: `boolean`
- **Required**: Required
- **Default Value**: *None*
- **Description**: If set to true, mixed encoding is restricted. This argument can be set to true to restrict the input if mixed encoding is detected. If this argument is set to true, and the given input is encoded using mixed encoding, an error will be thrown.

### Argument: `throwOnError`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: CF11+ If the value of this argument is true, and if restrictMultiple or restrictMixed is true and the given input contains mixed or multiple encoded strings, an exception will be thrown.
If the value of this argument is false, an empty string will be returned instead of an exception.

## Limitations and Other Info

- **Related Functions**: `encodeForCSS`, `encodeForXML`, `encodeForJavaScript`, `encodeForHTML`
- **Coldfusion Support**: Minimum version: `10`. Notes: ColdFusion 11: Added the new attribute, throwOnError.
- **Railo Support**: Minimum version: `4.0`.
- **Lucee Support**: Minimum version: `4.5`.
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: This BIF is provided by the `bx-esapi` module.

