# Function Name: `ArraySort`

## Description
Sorts array elements.

## Return Type
`boolean`

## Syntax
```cfml
arraySort(array, sortType [, sortOrder [, localeSensitive ]])
or
arraySort(array, callback)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of an array

### Argument: `sortType`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: numeric: sorts numbers
 text: sorts text alphabetically, taking case into account
 (also known as case-sensitive). All letters of one case
 precede the first letter of the other case:
 - aabzABZ, if sort_order = "asc" (ascending sort)
 - ZBAzbaa, if sort_order = "desc" (descending sort)

 textnocase: sorts text alphabetically, without regard to
 case (also known as case-insensitive). A letter in varying
 cases precedes the next letter:
 - aAaBbBzzZ, in an ascending sort; preserves original
 intra-letter order
 - ZzzBbBaAa, in a descending sort; reverses original
 intra-letter order

### Argument: `sortOrder`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `asc`
- **Description**: asc: ascending sort order. Default.
 - aabzABZ or aAaBbBzzZ, depending on value of sort_type,
 for letters
 - from smaller to larger, for numbers

 desc: descending sort order.
 - ZBAzbaa or ZzzBbBaAa, depending on value of sort_type,
 for letters
 - from larger to smaller, for numbers

### Argument: `callback`
- **Type**: `function`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ A function that uses two elements of an array. `function(element1, element2)`. Returns whether the first is less than (-1), equal to (0) or greater than (1) the second one (like the compare functions).

### Argument: `localeSensitive`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF10+ Specify if you wish to do a locale sensitive sorting.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `structSort`, `listSort`, `querySort`
- **Coldfusion Support**: Notes: CF2018+ Member function returns the sorted array.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

