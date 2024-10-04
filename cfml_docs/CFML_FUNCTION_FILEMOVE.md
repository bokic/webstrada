# Function Name: `FileMove`

## Description
Moves file from source to destination

## Return Type
`void`

## Syntax
```cfml
fileMove(source, destination)
```

## Arguments

### Argument: `source`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Path where the file is located currently

### Argument: `destination`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Path where a copy of the file should be placed

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Related Functions**: `fileWrite`, `fileDelete`, `directoryRename`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

