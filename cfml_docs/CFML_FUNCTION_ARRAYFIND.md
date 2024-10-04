# Function Name: `ArrayFind`

## Description
These functions searches the array for the specified value. Returns the index in the array of the first match, or 0 if there is no match.

## Return Type
`numeric`

## Syntax
```cfml
arrayFind(array, value)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The array you are searching.

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The value you are looking for in the array. CF10+ or Lucee4.5+ support passing a closure function in this argument as well.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayContains`, `arrayFind`, `arrayFindNoCase`, `arrayFindAll`, `arrayFindAllNoCase`
- **Coldfusion Support**: Minimum version: `9`. Notes: CF10+ Added support for closure instead of value
- **Lucee Support**: Notes: Lucee4.5+ Added support for closure instead of value
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

