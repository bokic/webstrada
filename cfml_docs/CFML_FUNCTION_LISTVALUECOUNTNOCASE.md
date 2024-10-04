# Function Name: `ListValueCountNoCase`

## Description
Counts instances of a specified value in a list.
The search is case-insensitive. For case-sensitive search use listValueCount.

## Return Type
`numeric`

## Syntax
```cfml
listValueCountNoCase(list, value [, delimiters])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `value`
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
- **Related Functions**: `listLen`, `structCount`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

