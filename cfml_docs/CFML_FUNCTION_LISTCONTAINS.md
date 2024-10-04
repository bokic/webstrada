# Function Name: `ListContains`

## Description
Determines the index of the first list element that contains a
 specified substring.
 Returns the index of the first list element that contains
 substring. If not found, returns zero. The search for the substring is case-sensitive.

## Return Type
`numeric`

## Syntax
```cfml
listContains(list, substring [, delimiters])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `substring`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: 

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `listContainsNoCase`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

