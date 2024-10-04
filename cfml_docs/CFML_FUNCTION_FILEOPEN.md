# Function Name: `FileOpen`

## Description
Opens a file

## Return Type
`any`

## Syntax
```cfml
fileOpen(filePath [, mode [, charset] [, seekable]])
```

## Arguments

### Argument: `filePath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An absolute path of an on-disk or in-memory file on the server

### Argument: `mode`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `read`
- **Description**: Type of access you require to the file stream

### Argument: `charset`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Character encoding identified by the file's byte order mark, if any; otherwise, JVM default file character set

### Argument: `seekable`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Whether the file is usable with the `fileSeek` function

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Related Functions**: `fileClose`, `fileCopy`, `fileReadBinary`, `fileRead`, `fileReadLine`, `fileWrite`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

