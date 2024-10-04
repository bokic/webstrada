# Function Name: `FileUploadAll`

## Description
Uploads all files sent to the page in an HTTP request to a directory on the server.

## Return Type
`array`

## Syntax
```cfml
fileUploadAll(destination [,mimeType] [,onConflict] [,strict] [,continueOnError] [,errorVariable] [,allowedExtensions])
```

## Arguments

### Argument: `destination`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Path of directory in which to upload the file. If not an absolute path (starting with a drive letter and a colon, or a forward or backward slash), it is relative to the ColdFusion temporary directory returned by the function getTempDirectory. If the destination you specify does not exist, ColdFusion creates a file with the specified destination name.

### Argument: `mimeType`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Limits the MIME types to accept. Comma-delimited list. For example, the following code permits JPEG and Microsoft Word file uploads:'image/jpg,application/msword' .The browser uses the filename extension to determine file type.

### Argument: `onConflict`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `error`
- **Description**: Action to take if file has the same name of a file in the directory.

### Argument: `strict`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: Defines which method is used to determine the file type to check against the value of the `mimeType` parameter.
`true`: The first few bytes of the uploaded file are used to determine the MIME type.
`false`: The MIME type provided by the browser in the request payload is used.

### Argument: `continueOnError`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF11+ Whether to continue uploading the remaining files when uploading one of the files fails.

### Argument: `errorVariable`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: `cffile.uploadAllErrors`
- **Description**: CF11+ The name of the variable in which the array of structs of file upload errors will be stored. 

The upload failure information error structure contains the following fields:
`REASON` - The reason for the failure
`DETAIL` - File upload failure detail
MESSAGE - A detailed message depicting the failure
CLIENTFILE - Name of the file uploaded from the client's system
`CLIENTFILEEXT` - Extension of the uploaded file on the client system (without a period)
`CLIENTFILENAME` - Name of the uploaded file on the client system (without an extension)
`INVALID_FILE_TYPE` - If the file mime type or extension is not in the specified accept attribute. If the reason is INVALID_FILE_TYPE, two additional keys will be available in the structure.
 --`ACCEPT`: list of mime types or file extensions given in the tag
--`MIMETYPE`: mime type of the uploaded file
`EMPTY_FILE` - If the uploaded file is an empty file
`FILE_EXISTS` - If any file with the given name already exists in the destination and the overwritepolicy is error
`DEST` - The destination where file is copied
`FORM_FILE_NOT_FOUND` - If the uploaded file is not found on the server

### Argument: `allowedExtensions`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF2018+ A comma-separated list of file extensions, which will be allowed for upload. 

For example, .png, .jpg, or, .jpeg. 

You can use `*` (star) to allow all files, except where you specify the MIME type in the accept attribute. 

Values specified in the attribute allowedExtensions override the list of blocked extensions in the server or application settings.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Related Functions**: `fileUpload`
- **Coldfusion Support**: Minimum version: `9.0.1`.
- **Lucee Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

