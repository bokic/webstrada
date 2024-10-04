# Function Name: `Evaluate`

## Description
Evaluates one or more string expressions, dynamically, from left to right. (The results of an evaluation on the left can have meaning in an expression to the right.) Returns the result of evaluating the rightmost expression.

## Return Type
`any`

## Syntax
```cfml
evaluate(expression)
```

## Arguments

### Argument: `expression`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Expression to evaluate. String expressions can be complex. If a string expression contains a single- or double-quotation mark, the mark must be escaped. This function is useful for forming one variable from multiple variables.

## Limitations and Other Info

- **Related Functions**: `de`, `render`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-unsafe-evaluate` module

