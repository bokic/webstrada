# Function Name: `GetCPUUsage`

## Description
 Gets the CPU usage with default or custom snapshot interval.

## Return Type
`numeric`

## Syntax
```cfml
getCpuUsage([interval]);
```

## Arguments

### Argument: `interval`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1000`
- **Description**: Time in milliseconds. This is the time delay between two snapshots.

## Limitations and Other Info

- **Related Functions**: `getSystemFreeMemory`, `getSystemTotalMemory`, `getMemoryUsage`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-oshi` module

