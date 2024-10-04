# Tag Name: `cfzip`

## Description
Manipulates ZIP and JavaTM Archive (JAR) files.
 In addition to the basic zip and unzip functions, use the cfzip tag to delete
 entries from an archive, filter files, read files in binary format, list the
 contents of an archive, and specify an entrypath used in an executable JAR file.

## Syntax
```cfml
<cfzip>
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `zip`
- **Description**: The action to take. Must be one of the following:
 delete
 list
 read
 readBinary
 unzip
 zip
 If you do not specify an action, ColdFusion
 applies the default action, zip. (optional)

### Attribute: `charset`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The character set used to translate the ZIP or JAR
 entry into a text string. Examples of character sets are:
 JIS
 RFC1345
 UTF-16 (optional, default=encoding of the host machine)

### Attribute: `destination`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Destination directory where the ZIP or JAR file is extracted. (optional)

### Attribute: `entrypath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Pathname on which the action is performed. (optional)

### Attribute: `file`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Absolute pathname of the file on which the action is performed.
 For example, the full pathname of the ZIP file: c:\temp\log.zip.
 If you do not specify the full pathname (for example, file="log.zip"),
 ColdFusion creates the file in a temporary directory. You can use the
 GetTempDirectory function to access the ZIP or JAR file. (required)

### Attribute: `filter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: File filter applied to the action. The action
 applies to all files in the pathname specified that match the filter. (optional)

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Record set name in which the result of the list action is stored.
 The record set columns are:
 name: filename of the entry in the JAR file. For example, if the entry is
 help/docs/index.htm, the name is index.htm.
 directory: directory containing the entry. For the example above, the
 directory is help/docs. You can obtain the full entry name by concatenating
 directory and name. If an entry is at the root level, the directory is empty ('').
 size: uncompressed size of the entry, in bytes.
 compressedSize: compressed size of the entry, in bytes.
 type: type of entry (directory or file).
 dateLastModified: last modified date of the entry, cfdate object.
 comment: any comment, if present, for the entry.
 crc: crc-32 checksum of the uncompressed entry data. (required)

### Attribute: `overwrite`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: unzip: Specifies whether to overwrite the extracted files:
 yes: if the extracted file already exists at the destination specified,
 the file is overwritten.
 no: if the extracted file already exists at the destination specified,
 the file is not overwritten and that entry is not extracted. The remaining
 entries are extracted.
 zip: Specifies whether to overwrite the contents of a ZIP or JAR file:
 yes: overwrites all of the content in the ZIP or JAR file if it exists.
 no: updates existing entries and adds new entries to the ZIP or JAR file
 if it exists. (optional, default=no)

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The password to be used for the archive when action is zip. (optional)

### Attribute: `prefix`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: String added as a prefix to the ZIP or JAR entry.
 The string is the name of a subdirectory in which the
 entries are added. (optional)

### Attribute: `recurse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies whether the action
 applies to subdirectories:
 yes: includes subdirectories.
 no: does not include subdirectories. (optional, default=yes)

### Attribute: `showDirectory`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: yes: lists the directories.
 no: does not list directories. (optional, default= no)

### Attribute: `source`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Source directory to be zipped. Not required
 if cfzipparam is specified. (required)

### Attribute: `storePath`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: zip: Specifies whether pathnames are stored in the ZIP or JAR file:
 yes: pathnames of entries are stored in the ZIP or JAR file.
 no: pathnames of the entries are not stored in the ZIP or JAR file.
 All the files are placed at the root level. In case of a name conflict,
 the last file in the iteration is added.
 unzip: Specifies whether files are stored at the entrypath:
 yes: the files are extracted to the entrypath.
 no: the entrypath is ignored and all the files are extracted
 at the root level. (optional, default= yes)

### Attribute: `variable`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Variable in which the read content is stored. (required)

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

