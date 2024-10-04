# Function Name: `ListSort`

## Description
Sorts list elements according to a sort type and sort order. Returns a sorted copy of the list.
 [sortType - description]
 numeric: sorts numbers
 text: sorts text alphabetically, taking case into account
 - aabzABZ, if sort_order = "asc"
 - ZBAzbaa, if sort_order = "desc"
 textnocase: sorts text alphabetically, without regard to case
 - aAaBbBzzZ, in an asc sort;
 - ZzzBbBaAa, in a desc sort;

## Return Type
`string`

## Syntax
```cfml
listSort(list, sortType [, sortOrder] [, delimiters] [, includeEmptyFields] [, localeSensitive])
or
listSort(list, callback)
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A list or variable name

### Argument: `sortType`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: numeric: sorts numbers
 text: sorts text alphabetically, taking case into account
 (also known as case-sensitive).
 - aabzABZ for ascending sort(sort_order = "asc")
 - ZBAzbaa for descending sort(sort_order = "desc")

 textnocase: sorts text alphabetically, without regard to
 case (also known as case-insensitive).
 - aAaBbBzzZ for ascending sort(sort_order = "asc")
 - ZzzBbBaAa for descending sort(sort_order = "desc")

### Argument: `sortOrder`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `asc`
- **Description**: - asc: ascending sort order
- desc: descending sort order

### Argument: `delimiters`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: Characters that separate the list elements. The default value is comma. If this parameter contains more than one character, ColdFusion uses the first character in the string as the delimiter in the output list.

### Argument: `includeEmptyFields`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Set to true to include empty fields.

### Argument: `callback`
- **Type**: `function`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF2016+ A function that uses two elements of a list. `function(element1, element2)`. Returns whether the first is less than (-1), equal to (0) or greater than (1) the second one (like the compare functions).

### Argument: `localeSensitive`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF10+ Specify if you wish to do a locale sensitive sorting.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `structSort`, `arraySort`, `querySort`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

