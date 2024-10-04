# Function Name: `DirectoryExists`

## Description
Determines whether an on-disk or in-memory directory exists.

## Return Type
`boolean`

## Syntax
```cfml
directoryExists(path)
```

## Arguments

### Argument: `path`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An absolute on-disk or in-memory path. Alternatively, you can specify IP address as in the following example: `DirectoryExists("//12.3.123.123/c_drive/test");`

### Argument: `allowRealPath `
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee Only. boolean that defines if relative paths are interpreted or not.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions for directory operations.
- **Related Functions**: `directoryCreate`, `directoryDelete`, `fileExists`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

