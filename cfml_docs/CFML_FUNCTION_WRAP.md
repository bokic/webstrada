# Function Name: `Wrap`

## Description
 Wraps text so that each line has a specified maximum number
 of characters.

## Return Type
`string`

## Syntax
```cfml
wrap(String, limit [, strip])
```

## Arguments

### Argument: `String`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String or variable that contains one. The text to wrap.

### Argument: `limit`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Positive integer maximum number of characters to allow on
 a line.

### Argument: `strip`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Whether to remove all existing newline and carriage return
 characters in the input string with spaces before wrapping
 the text. Default: False.

## Limitations and Other Info

- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

