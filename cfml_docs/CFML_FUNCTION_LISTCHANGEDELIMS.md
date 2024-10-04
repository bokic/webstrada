# Function Name: `ListChangeDelims`

## Description
 Changes a list delimiter.
 Returns a copy of the list, with each delimiter character
 replaced by new_delimiter.

## Return Type
`string`

## Syntax
```cfml
listChangeDelims(list, new_delimiter [, delimiters, [includeEmptyValues]])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A list or a variable that contains one.

### Argument: `new_delimiter`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Delimiter string or a variable that contains one. Can be an empty string. ColdFusion processes the string as one delimiter.

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: A string or a variable that contains one. Characters that separate list elements. The default value is comma. If this parameter contains more than one character, ColdFusion processes each occurrence of each character as a delimiter.

### Argument: `includeEmptyValues`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `NO`
- **Description**: CF10+ Set to yes to include empty values.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `ListFirst, ListQualify`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

