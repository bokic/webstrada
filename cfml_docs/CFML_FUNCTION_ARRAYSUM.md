# Function Name: `ArraySum`

## Description
The sum of values in an array. If the array parameter value is
 an empty array, returns zero.

## Return Type
`numeric`

## Syntax
```cfml
arraySum(array)
```

## Arguments

### Argument: `array`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An array name or variable name

### Argument: `ignoreEmpty`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: CF2016.0.3+ Whether to ignore empty string or null values

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid array. Standard array functions in CFML are 1-indexed.
- **Related Functions**: `arrayReduce`
- **Coldfusion Support**:
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

