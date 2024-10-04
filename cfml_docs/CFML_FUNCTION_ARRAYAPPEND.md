# Function Name: `ArrayAppend`

## Description
Appends an element to the end of an array.

## Return Type
`boolean`

## Syntax
```cfml
arrayAppend(array, value [, merge])
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The array to which the element should be appended.

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The element to append. Can be any type.

### Argument: `merge`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: CF10+ When true appends array elements individually to the specified array. When false (default), the new array is appended as a single element.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayprepend`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**: Notes: When called as a member function array.append() returns the array instead of boolean.
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

