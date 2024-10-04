# Function Name: `FindOneOf`

## Description
Finds the first occurrence of any one of a set of characters in a string, from a specified start position. The search is case-sensitive.

 Returns the position of the first member of set found in string; or 0, if no member of set is found in string.

## Return Type
`numeric`

## Syntax
```cfml
findOneOf(set, string [, start])
```

## Arguments

### Argument: `set`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String which contains one or more characters to search for.

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String in which to search.

### Argument: `start`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: Start position of search. Can be 1 through the length of the string to search. Choosing a start index greater than the length of the string to search will return a 0.

## Limitations and Other Info

- **Related Functions**: `listFind`, `find`, `findNoCase`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

