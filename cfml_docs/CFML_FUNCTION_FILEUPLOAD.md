# Function Name: `FileUpload`

## Description
Uploads file to a directory on the server.

## Return Type
`struct`

## Syntax
```cfml
fileUpload(destination [, fileField] [, mimeType] [, onConflict] [, strict])
```

## Arguments

### Argument: `destination`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Path of directory in which to upload the file. If not an absolute path (starting with a drive letter and a colon, or a forward or backward slash), it is relative to the ColdFusion temporary directory returned by the function getTempDirectory. If the destination you specify does not exist, ColdFusion creates a file with the specified destination name.

### Argument: `fileField`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of form field used to select the file. Do not use number signs (#) to specify the field name.

### Argument: `mimeType`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify a comma-delimited list of MIME types and/or file extensions to test the uploaded file against. If the file is not of any of the types in this list, an error of type `coldfusion.tagext.io.FileUtils$InvalidUploadTypeException`is thrown.
If you specify file extensions, use this format: `.txt,.jpg`, not `txt`, `*.txt`, or `*.*`. You can use `*` as a wildcard to accept all files.

### Argument: `onConflict`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `error`
- **Description**: Action to take if file has the same name of a file in the directory.

### Argument: `strict`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: CF10+ Defines which method is used to determine the file type to check against the value of the `mimeType` parameter.
`true`: The first few bytes of the uploaded file are used to determine the MIME type.
`false`: The MIME type provided by the browser in the request payload is used.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Related Functions**: `fileUploadAll`
- **Coldfusion Support**: Minimum version: `9.0.1`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

