# Function Name: `GetSystemTotalMemory`

## Description
 Gets details of the memory that is available for the operating system, in bytes.

## Return Type
`numeric`

## Syntax
```cfml
getSystemTotalMemory();
```

## Arguments

### Argument: `region`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Indicates the cache region from which to remove the stored objects. If no value is specified, default cache region is considered by default.

## Limitations and Other Info

- **Related Functions**: `getSystemFreeMemory`, `getMemoryUsage`, `getCpuUsage`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-ini` module.

