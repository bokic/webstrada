# Function Name: `ListLast`

## Description
Gets the last element of a list.

## Return Type
`string`

## Syntax
```cfml
listLast(list [, delimiters, includeEmptyValues ])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A list or a variable that contains a list.

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: A string or a variable that contains one. Characters that separate list elements. The default value is comma. If this parameter contains more than one character, ColdFusion processes each occurrence of each character as a delimiter; you cannot specify a multicharacter delimiter.

### Argument: `includeEmptyValues`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Set to true to include empty values.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `listFirst`, `listRest`, `listGetAt`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

