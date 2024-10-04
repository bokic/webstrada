# Function Name: `FileRead`

## Description
Reads an on-disk or in-memory text file or a file object created with the FileOpen function.

## Return Type
`string`

## Syntax
```cfml
fileRead(filePath [, charset | bufferSize])
```

## Arguments

### Argument: `filePath`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An absolute file path, or file object.

### Argument: `charset`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Character encoding used to read the file.

### Argument: `bufferSize`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The number of characters to read.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Related Functions**: `filewrite`, `filereadline`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

