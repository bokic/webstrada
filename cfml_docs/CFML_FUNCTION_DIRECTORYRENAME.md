# Function Name: `DirectoryRename`

## Description
Renames given directory. NOTE:Ensure that you have the required permissions to run this function.

## Return Type
`void`

## Syntax
```cfml
directoryRename(oldPath, newPath)
```

## Arguments

### Argument: `oldPath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Absolute path of the directory to be renamed. Alternatively, you can specify IP address, for example, `DirectoryRename("//12.3.123.123/c_drive/test");`

### Argument: `newPath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: New name for the directory.

### Argument: `createPath`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: Lucee Only. If set to `false`, expects all parent directories to exist. `true` will generate necessary directories.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions for directory operations.
- **Related Functions**: `fileMove`, `directoryCreate`, `directoryDelete`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

