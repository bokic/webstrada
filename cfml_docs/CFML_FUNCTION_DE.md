# Function Name: `DE`

## Description
Delay evaluation of a string as an expression, when it is passed as a parameter to the IIf or Evaluate functions. Escapes any double quotation marks in the parameter and wraps the result in double quotation marks. It does not escape `#` so the string could still be evaluated in some cases.

## Return Type
`string`

## Syntax
```cfml
de(String)
```

## Arguments

### Argument: `String`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

## Limitations and Other Info

- **Related Functions**: `iif`, `evaluate`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

