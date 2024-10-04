# Function Name: `ArrayDelete`

## Description
Deletes the first element in an array that matches the value of `value`.
The search is case-sensitive.
Returns `true` if the element was found and removed.
The array will be resized, so that the deleted element doesn't leave a gap.

## Return Type
`boolean`

## Syntax
```cfml
arrayDelete(array, value)
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
- **Description**: A value for which to search. Case-sensitive.

### Argument: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `one`
- **Description**: Lucee4.5+ remove one (default) or all occurrences of the value

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayDeleteAt`, `arrayDeleteNoCase`
- **Coldfusion Support**:
- **Lucee Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

