# Function Name: `ArrayFindAll`

## Description
Searches an array for all positions of a specified value. The function searches for simple values such as strings and numbers or for complex objects such as structures. When the second parameter is a simple value, string searches are case-sensitive

## Return Type
`array`

## Syntax
```cfml
arrayFindAll(array, value or callback)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The source array to search through

### Argument: `value or callback`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If string, case-sensitive value to search for; if callback, use signature function (item, index, array) : boolean

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayContains`, `arrayFind`, `arrayFindNoCase`, `arrayFindAll`, `arrayFindAllNoCase`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

