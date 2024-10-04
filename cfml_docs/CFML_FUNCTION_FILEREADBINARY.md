# Function Name: `FileReadBinary`

## Description
Reads an on-disk or in-memory binary file (such as an executable or image file) on the server, into a binary object

## Return Type
`binary`

## Syntax
```cfml
fileReadBinary(filePath)
```

## Arguments

### Argument: `filePath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Absolute path to the file.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Related Functions**: `fileRead`, `fileReadLine`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

