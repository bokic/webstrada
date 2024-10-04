# Function Name: `StructSort`

## Description
Returns a sorted array of the top level keys in a structure. Sorts using alphabetic or numeric sorting, and can sort based on the values of any structure element.

## Return Type
`array`

## Syntax
```cfml
structSort(struct [, sortType, sortOrder, path, localeSensitive])
structSort(struct, callback)
```

## Arguments

### Argument: `struct`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A ColdFusion structure

### Argument: `sortType`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `text`
- **Description**: * numeric
* text: case-sensitive
* textnocase

### Argument: `sortOrder`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `asc`
- **Description**: * asc: ascending (a to z) sort order.
* desc: descending (z to a) sort order

### Argument: `path`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Top-level key path; String or a variable that contains one

### Argument: `localeSensitive`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: CF10+ Respect locale-specific characters (including support for umlaut characters) while sorting
(Applies to type"text" and "textnocase".

### Argument: `callback`
- **Type**: `function`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF2016+ A closure for sorting which takes two keys of the struct and returns whether the first value is greater than, equal to, or less than the second value. Inside compare function can be used (compare, compareNoCase, dateCompare or custom). `function(key1, key2)`

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Related Functions**: `arraySort`, `listSort`, `querySort`
- **Coldfusion Support**: Notes: CF10+ Added support for locale-specific characters
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

