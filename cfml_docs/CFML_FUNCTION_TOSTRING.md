# Function Name: `ToString`

## Description
 Converts a value to a string.
Lucee parses numbers with one decimal place.
complex object types can only be used in combination with the member syntax.

## Return Type
`string`

## Syntax
```cfml
toString(any_value [, encoding])
```

## Arguments

### Argument: `any_value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Value to convert to a string

### Argument: `encoding`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The character encoding (character set) of the string.

The default value is the encoding of the page on which the function is called.

## Limitations and Other Info

- **Related Functions**: `Val`, `NumberFormat`
- **Coldfusion Support**: Minimum version: `4.5`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

