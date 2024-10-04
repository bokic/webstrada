# Function Name: `ArrayFindNoCase`

## Description
These functions performs a case-insensitive search in the array for the specified value. Returns the array index of the first match; 0 if not found.

## Return Type
`numeric`

## Syntax
```cfml
arrayFindNoCase(array, value or callback)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The array to search

### Argument: `value or callback`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The value you are looking for in the array.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayContains`, `arrayFind`, `arrayFindNoCase`, `arrayFindAll`, `arrayFindAllNoCase`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

