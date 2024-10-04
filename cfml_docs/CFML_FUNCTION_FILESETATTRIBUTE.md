# Function Name: `FileSetAttribute`

## Description
Sets the attributes of an on-disk file in Windows. This function does not work with in-memory files.

## Return Type
`void`

## Syntax
```cfml
fileSetAttribute(filePath, attribute)
```

## Arguments

### Argument: `filePath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Path to on-disk file

### Argument: `attribute`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Attribute to enable/disable

readonly/hidden sets the given attribute to `true` and the other one to `false`
normal sets both to false

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Related Functions**: `getFileInfo`, `fileWrite`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

