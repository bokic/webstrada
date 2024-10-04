# Function Name: `RandRange`

## Description
Generates a random integer between two specified numbers.
 Requests for random integers that are greater than 100,000,000
 result in non-random numbers, to prevent overflow during
 internal computations.

## Return Type
`numeric`

## Syntax
```cfml
randRange(number1, number2 [, algorithm])
```

## Arguments

### Argument: `number1`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `number2`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `algorithm`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `CFMX_COMPAT`
- **Description**: CF7+ The algorithm to use to generated the random number.

## Limitations and Other Info

- **Related Functions**: `Rand`, `Randomize`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

