# Function Name: `CallStackDump`

## Description
Similar to the function callStackGet except that it outputs a string representation of the call stack.

## Return Type
`void`

## Syntax
```cfml
callStackDump(output)
```

## Arguments

### Argument: `output`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `browser`
- **Description**: If you chose "file" and do not provide the complete path to the file, the file is written to the temp directory as determined by the function `getTempDirectory()`.

## Limitations and Other Info

- **Related Functions**: `callStackGet`, `cfdump`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Railo Support**:

