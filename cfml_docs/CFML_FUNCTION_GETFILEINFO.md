# Function Name: `GetFileInfo`

## Description
Returns information about on-disk or in-memory file. Return struct contains keys such as: lastModified, size, path, name, type, canWrite, canRead, isHidden and more.

## Return Type
`struct`

## Syntax
```cfml
getFileInfo(path)
```

## Arguments

### Argument: `path`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Path to the on-disk or in-memory file

## Limitations and Other Info

- **Related Functions**: `fileRead`, `cffile`, `fileGetMimeType`, `getFileFromPath`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

