# Function Name: `ArrayContains`

## Description
Used to determine if a value is in the given array, case-sensitive. Adobe CF and OpenBD return boolean. Lucee / Railo returns the numeric index if the value is found, 0 if not.

## Return Type
`boolean`

## Syntax
```cfml
arrayContains(array, value)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The array in which to search.

### Argument: `value`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The value to search for in the array.

### Argument: `substringMatch`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Lucee4.5+ If set to true then a substring match will also return an array position. This will only work with simple values. Passing true with complex objects will throw an exception.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayContains`, `arrayFind`, `arrayFindNoCase`, `arrayFindAll`, `arrayFindAllNoCase`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**: Notes: Returns the position of the array instead of boolean value.
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

