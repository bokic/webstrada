# Function Name: `DirectoryCreate`

## Description
Creates an on-disk or in-memory directory in the specified path

## Return Type
`void`

## Syntax
```cfml
directoryCreate(path)
```

## Arguments

### Argument: `path`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Absolute path of the directory to be created. Alternatively, you can specify an IP address, as in the following example: `DirectoryCreate("//12.3.123.123/c_drive/test". NOTE: You have to have the required permissions to run this function.`);

### Argument: `createPath`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: Lucee Only. Create parent directory when it doesn't exist.

### Argument: `ignoreExists`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Lucee Only. Pass false (default) to throw an error if the directory already exists, or true to skip the create operation without an error.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions for directory operations.
- **Related Functions**: `fileWrite`, `directoryDelete`, `directoryExists`, `directoryRename`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

