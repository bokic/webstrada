# Function Name: `FileGetMimeType`

## Description
Gets the MIME type for the file path/file object you have specified.

## Return Type
`string`

## Syntax
```cfml
fileGetMimeType(file, strict)
```

## Arguments

### Argument: `file`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the file object or full path on disk to the file if strict is set to true. If you do not specify the full path, the file is assumed to be present in the temp directory, as returned by the function getTempDirectory.

### Argument: `strict`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: If false, determines the file type by extension. The default value is true.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

