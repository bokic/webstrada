# Function Name: `DirectoryList`

## Description
List the contents of a directory. Returns either an array, or a query. NOTE: Ensure that you have the required permissions to run this function.

## Return Type
`any`

## Syntax
```cfml
directoryList(path [, recurse] [, listInfo] [, filter] [, sort] [, type])
```

## Arguments

### Argument: `path`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The absolute path of the directory for which to list the contents. Alternatively, you can specify IP address as in the following example: `DirectoryList("//12.3.123.123/c_drive/test");`.

### Argument: `recurse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: If `true` directoryList traverses the directory tree.

### Argument: `listInfo`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `path`
- **Description**: Sets the return type. `name` returns an array with only the file names, `path` returns an array with the full path names and `query` returns a query containing the following fields: `Attributes`, `DateLastModified`, `Directory`, `Link`, `Mode`, `Name`, `Size`, `Type`.

### Argument: `filter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: File extension filter applied to the listed files, for example, `*.jpg`. Multiple filters can be applied by using a pipe delimiter. For example: `*.doc|*.xls`. You can also pass a function. The arguments of the passed function must have: `path` :the file path, `type`: The values (file or dir), `extension`: The file extension, if any, otherwise and empty string. This argument can also accept the instances of Java `FileFilter` Objects. In Lucee4.5+ it can be a closure as well.

### Argument: `sort`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Columns by which to sort. e.g. `Directory, Size DESC, DateLastModified`. To qualify a column, use `asc` (ascending sort a-z) or `desc` (descending sort z-a).

### Argument: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `all`
- **Description**: CF11+ Lucee5+ Filter the result to only include files, directories, or both.

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions for directory operations.
- **Related Functions**: `cfdirectory`
- **Coldfusion Support**: Minimum version: `9`. Notes: Only CF11+ supports the type argument.
- **Lucee Support**: Notes: In Lucee the `filter` param can be a closure as well where the path is passed in. The `sort` argument only works when `listInfo="query"`.
- **Railo Support**: Notes: If a directory returns false in recursive mode the contained files are processed anyways
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

