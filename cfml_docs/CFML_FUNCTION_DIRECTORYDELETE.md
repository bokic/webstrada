# Function Name: `DirectoryDelete`

## Description
Deletes on-disk or in-memory directory at the given path. NOTE: Ensure that you have the required permissions to run this function.

## Return Type
`void`

## Syntax
```cfml
directoryDelete(path[, recurse])
```

## Arguments

### Argument: `path`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Absolute path of the directory to be deleted. Alternatively, you can specify IP address, as in the following example: `DirectoryDelete("//12.3.123.123/c_drive/test");`.

### Argument: `recurse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: If true, the directory and the sub-directories are deleted. If the directory (being deleted) has sub-directories and you set `recurse` to false, an exception occurs.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions for directory operations.
- **Related Functions**: `directoryCreate`, `directoryRename`, `fileDelete`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

