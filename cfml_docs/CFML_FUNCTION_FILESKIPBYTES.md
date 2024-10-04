# Function Name: `FileSkipBytes`

## Description
Shifts the file pointer by the given number of bytes.

## Return Type
`void`

## Syntax
```cfml
fileSkipBytes(file, skipCount)
```

## Arguments

### Argument: `file`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The file object

### Argument: `skipCount`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of bytes that must be skipped before the next file operation

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

