# Function Name: `HTMLEditFormat`

## Description
Replaces special characters in a string with their HTML-escaped equivalents.

## Return Type
`string`

## Syntax
```cfml
htmlEditFormat( string [, version] )
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string or a variable that contains one.

### Argument: `version`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `2.0`
- **Description**: HTML version to use; currently ignored.

## Limitations and Other Info

- **Related Functions**: `encodeForHTML`
- **Coldfusion Support**: Notes: Use encodeForHTML, which can provide more protection from XSS.
- **Lucee Support**:
- **Openbd Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

