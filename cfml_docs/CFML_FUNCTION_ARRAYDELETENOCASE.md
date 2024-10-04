# Function Name: `ArrayDeleteNoCase`

## Description
Deletes the first element in an array that matches the value of `value`. The search is case insensitive. Returns `true` if the element was found and removed. The array will be resized, so that the deleted element doesn't leave a gap.

## Return Type
`boolean`

## Syntax
```cfml
arrayDeleteNoCase(array, value)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A value for which to search. Case insensitive.

### Argument: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `one`
- **Description**: Lucee5.1+ Remove one (default) or all occurrences of the value.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayDeleteAt`, `arrayDelete`
- **Coldfusion Support**: Minimum version: `2016`.
- **Lucee Support**: Minimum version: `5.1.0`.
- **Boxlang Support**: Minimum version: `1.0.0`.

