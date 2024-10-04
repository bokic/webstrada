# Function Name: `FileSeek`

## Description
Shifts the file pointer to the given position. The file must be opened with seekable option

## Return Type
`void`

## Syntax
```cfml
fileSeek(file, position)
```

## Arguments

### Argument: `file`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The file object

### Argument: `position`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The position in the file within a stream where the following read and write operation must occur.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

