# Function Name: `ThreadJoin`

## Description
Waits for the given thread object to finish running

## Return Type
`void`

## Syntax
```cfml
threadJoin()
```

## Arguments

### Argument: `threadName`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Thread to join

If no `threadName` is specified, all running threads will be affected.
You can pass a threadname as string or multiple threads as comma-separated list

### Argument: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: The number of milliseconds for which to suspend thread processing

## Limitations and Other Info

- **Related Functions**: `cfthread`, `threadTerminate`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

