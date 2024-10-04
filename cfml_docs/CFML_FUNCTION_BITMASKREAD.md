# Function Name: `BitMaskRead`

## Description
Performs a bitwise mask read operation.
 Returns an integer representation of the corresponding bits specified in the mask.

## Return Type
`numeric`

## Syntax
```cfml
bitMaskRead(number, start, length)
```

## Arguments

### Argument: `number`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Integer

### Argument: `start`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Start bit for the read mask (Integer in the range 0-31, inclusive)

### Argument: `length`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Length of bits in the read mask (Integer in the range 0-31, inclusive)

## Limitations and Other Info

- **Related Functions**: `bitMaskClear`, `bitMaskSet`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

