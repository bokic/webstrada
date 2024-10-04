# Function Name: `PrecisionEvaluate`

## Description
 Evaluates one or more string expressions using BigDecimal precision arithmetic. If the results ends in an infinitely repeating decimal value only the first 20 digits of the decimal portion will be used.  BigDecimal precision results only work with addition, subtraction, multiplication and division.  The use of ^, MOD, % or \ arithmetic operators will result in normal integer precision.

## Return Type
`numeric`

## Syntax
```cfml
precisionEvaluate(expressions)
```

## Arguments

### Argument: `expressions`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Expressions to evaluate

## Limitations and Other Info

- **Related Functions**: `evaluate`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

