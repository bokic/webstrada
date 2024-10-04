# Function Name: `StructToSorted`

## Description
Converts a struct to an ordered one

## Return Type
`struct`

## Syntax
```cfml
structToSorted(structure, callback)
structToSorted(structure, sorttype, sortorder, localeSensitive)
```

## Arguments

### Argument: `structure`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Structure or a variable that contains one.

### Argument: `callback`
- **Type**: `function`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Closure or function reference that will be called for each iteration. Should return -1, 0 or 1.

### Argument: `sorttype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `text`
- **Description**: 

### Argument: `sortorder`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `asc`
- **Description**: * asc: ascending (a to z) sort order.
* desc: descending (z to a) sort order

### Argument: `localeSensitive`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Respect locale-specific characters (including support for umlaut characters) while sorting (applies to type"text").

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Related Functions**: `structSort`
- **Coldfusion Support**: Minimum version: `2016.0.3`.
- **Boxlang Support**: Minimum version: `1.0.0`.

