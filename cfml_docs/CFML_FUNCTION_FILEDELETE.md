# Function Name: `FileDelete`

## Description
Deletes the specified file on the server. fileDelete throws an exception whenever a file doesn't exist.

## Return Type
`void`

## Syntax
```cfml
fileDelete(filePath)
```

## Arguments

### Argument: `filePath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Pathname of the file to delete. If not an absolute path (starting with a drive letter and a colon, or a forward or backward slash), it is relative to the ColdFusion temporary directory, which is returned by the GetTempDirectory function.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Related Functions**: `fileWrite`, `directoryDelete`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

