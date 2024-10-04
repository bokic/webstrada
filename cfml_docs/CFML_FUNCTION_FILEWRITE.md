# Function Name: `FileWrite`

## Description
Writes the data to the file object or file path specified using the charset specified or the java default character set if unspecified.

## Return Type
`void`

## Syntax
```cfml
fileWrite(filePath, data [, charset])
```

## Arguments

### Argument: `filePath`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A file object or a file system path string.

### Argument: `data`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The variable to  write to the file.

### Argument: `charset`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: An optional character set that the data is encoded with. Defaults to the Java default character set (which is usually UTF-8).

## Limitations and Other Info

- **Permission/IO**: Requires appropriate file system read/write permissions on the target directory/path.
- **Related Functions**: `cffile`, `fileAppend`, `fileWriteLine`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

