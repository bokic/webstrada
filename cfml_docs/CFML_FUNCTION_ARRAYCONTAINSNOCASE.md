# Function Name: `ArrayContainsNoCase`

## Description
Used to determine if a value is in the given array, case insensitive.

## Return Type
`boolean`

## Syntax
```cfml
arrayContainsNoCase(array, value)
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
- **Description**: 

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayContains`, `arrayFind`, `arrayFindNoCase`, `arrayFindAll`, `arrayFindAllNoCase`
- **Lucee Support**: Notes: Returns index of first occurrence
- **Railo Support**:
- **Openbd Support**:
- **Coldfusion Support**: Minimum version: `2016`. Notes: Member function only available in CF2021+
- **Boxlang Support**: Minimum version: `1.0.0`.

