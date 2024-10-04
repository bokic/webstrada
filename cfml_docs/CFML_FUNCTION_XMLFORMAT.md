# Function Name: `XmlFormat`

## Description
Escapes XML special characters in a string, so that the string is safe to use with XML.

## Return Type
`string`

## Syntax
```cfml
xmlFormat(String [, escapeChars])
```

## Arguments

### Argument: `String`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The string to escape

### Argument: `escapeChars`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: When true escapes restricted characters according to the W3C XML standard.

## Limitations and Other Info

- **Type Requirement**: Operates on XML document objects or XML markup strings.
- **Related Functions**: `encodeForXML`
- **Coldfusion Support**: Minimum version: `4.5`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

