# Function Name: `ThreadTerminate`

## Description
Terminates the thread specified by the threadName parameter.

## Return Type
`void`

## Syntax
```cfml
threadTerminate(threadname)
```

## Arguments

### Argument: `threadname`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Thread to be terminated

## Limitations and Other Info

- **Related Functions**: `cfthread`, `threadJoin`
- **Coldfusion Support**:
- **Lucee Support**: Notes: In Lucee the default for threadname parameter is "run" and the terminated thread scope will have an error object included after termination.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

