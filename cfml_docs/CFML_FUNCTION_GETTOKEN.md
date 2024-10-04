# Function Name: `GetToken`

## Description
 Determines whether a token of the list in the delimiters
 parameter is present in a string.
 Returns the token found at position index of the string, as a
 string. If index is greater than the number of tokens in the
 string, returns an empty string.

## Return Type
`string`

## Syntax
```cfml
getToken(string, index [, delimiters])
```

## Arguments

### Argument: `String`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `index`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: 

## Limitations and Other Info

- **Related Functions**: `Left`, `Right`, `Mid`, `SpanExcluding`, `SpanIncluding`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

