# Function Name: `DirectoryCopy`

## Description
Copies the contents of a directory to a destination directory

## Return Type
`void`

## Syntax
```cfml
directoryCopy(source, destination [, recurse][, filter])
```

## Arguments

### Argument: `source`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Absolute pathname of directory from which you copy content.

### Argument: `destination`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Path of the destination directory. If not an absolute path, it is relative to the source directory.

### Argument: `recurse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: If true, copies the subdirectories, otherwise only the files in the source directory.

### Argument: `filter`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: File extension filter applied, for example, *.cfm. Filter to be used to filter the data copied: - A string that uses "*" as a wildcard, for example, "*.cfm" - a UDF (User defined Function) using the following pattern "functioname(String path):boolean", the function is run for every single file, if the function returns true, then the file is will be added to the list otherwise it will be omitted.

### Argument: `createPath`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: Lucee4.5+ If set to false, expects all parent directories to exist. If set to true, it will generate necessary directories.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions for directory operations.
- **Related Functions**: `directorylist`, `directoryrename`, `directoryexists`, `directorydelete`, `directorycreate`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

