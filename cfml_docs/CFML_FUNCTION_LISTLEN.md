# Function Name: `ListLen`

## Description
 Determines the number of elements in a list.

## Return Type
`numeric`

## Syntax
```cfml
listLen(list [, delimiters, [includeEmptyValues]])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A list or a variable that contains one

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: A string or a variable that contains one. Characters that separate list elements. The default value is comma. If this parameter contains more than one character, ColdFusion processes each occurrence of each character as a delimiter.

### Argument: `includeEmptyValues`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `NO`
- **Description**: CF10+ If includeEmptyValues is set to true, all empty values in the list will be considered when computing length. If set to false, the empty list elements are ignored.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `len`, `arrayLen`, `structCount`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

