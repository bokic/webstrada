# Function Name: `ReplaceList`

## Description
Replaces occurrences of the elements from a delimited list
 in a string with corresponding elements from another delimited
 list. The search is case-sensitive.

## Return Type
`string`

## Syntax
```cfml
replaceList(String, list1, list2 [, includeEmptyFields])
replaceList(String, list1, list2, delimiter [, includeEmptyFields])
replaceList(String, list1, list2, delimiterList1, delimiterList2 [, includeEmptyFields])
```

## Arguments

### Argument: `String`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string, or a variable that contains one, within which to replace substring

### Argument: `list1`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: List of substrings for which to search

### Argument: `list2`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: List of replacement substrings

### Argument: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: Common delimiter for both search and replacement.

### Argument: `delimiterList1`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: Delimiter for search.

### Argument: `delimiterList2`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: Delimiter for replacement.

### Argument: `includeEmptyFields`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: When true, zero-length list elements are preserved.

## Limitations and Other Info

- **Related Functions**: `replaceListNoCase`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

