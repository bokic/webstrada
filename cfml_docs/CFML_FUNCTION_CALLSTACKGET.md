# Function Name: `CallStackGet`

## Description
Returns an array of structs by default. Each struct contains template name, line number, and function name (if applicable). This is a snapshot of all function calls or invocations.

## Return Type
`any`

## Syntax
```cfml
callStackGet()
```

## Arguments

### Argument: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `array`
- **Description**: Lucee4.5+ The type of the returned value

### Argument: `offset`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Lucee5.3.8+ The number of frames to skip

### Argument: `maxFrames`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Lucee5.3.8+ The maximum number of frames to return

## Limitations and Other Info

- **Related Functions**: `callStackDump`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

