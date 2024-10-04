# Function Name: `FileSetLastModified`

## Description
Sets the date when an on-disk or in-memory file was most recently modified.

## Return Type
`void`

## Syntax
```cfml
fileSetLastModified(filePath, date)
```

## Arguments

### Argument: `filePath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An absolute path to an on-disk or in-memory file on the server

### Argument: `date`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The date to set for when the file was last modified

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

