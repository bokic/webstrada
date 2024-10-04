# Function Name: `ListQualify`

## Description
Inserts a string at the beginning and end of list elements.

## Return Type
`string`

## Syntax
```cfml
listQualify(list, qualifier [, delimiters] [, elements] [, includeEmptyValues])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A list or variable name

### Argument: `qualifier`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string or character in which to insert before and after the list elements

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: Characters that separate list elements. The default value is comma. If this parameter contains more than one character, ColdFusion uses the first character as the delimiter and ignores the remaining characters.

### Argument: `elements`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `all`
- **Description**: all -all elements; char -elements that are composed of alphabetic characters

### Argument: `includeEmptyFields`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If includeEmptyFields is true, empty value add in list elements

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `listCompact`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

