# Function Name: `IIf`

## Description
A boolean condition or value is passed into the first argument. If the condition is `true` the second argument is evaluated and returned, if `false` the third argument is evaluated and returned.

## Return Type
`string`

## Syntax
```cfml
iIf(condition, expression1, expression2)
```

## Arguments

### Argument: `condition`
- **Type**: `boolean`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A boolean value or an expression that evaluates to a boolean.

### Argument: `expression1`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A CFML expression that is evaluated dynamically using Evaluate if the condition is `true`.

### Argument: `expression2`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A CFML expression that is evaluated dynamically using Evaluate if the condition is `false`.

## Limitations and Other Info

- **Related Functions**: `evaluate`, `de`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

