# Tag Name: `cfzipparam`

## Description
Provides additional information to the cfzip tag.
 The cfzipparam tag is always a child tag of the cfzip tag.

## Syntax
```cfml
<cfzipparam>
```

## Attributes / Variants

### Attribute: `charset`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `encoding of the host machine`
- **Description**: Converts string content into binary data before putting
 it into a ZIP or JAR file.

Used only when cfzip
 action="zip" and the cfzipparam content is a string

### Attribute: `content`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Content written to the ZIP or JAR entry.

Used only when cfzip action="zip".
 Valid content data types are binary and string. If you specify the content
 attribute, you must specify the entrypath attribute.

### Attribute: `entrypath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Pathname used:
 For cfzip action="zip", it is the entrypath used. This is valid only
 when the source is a file. The entrypath creates a subdirectory within
 the ZIP or JAR file.
 For cfzip action="unzip", it is the pathname to unzip.
 For cfzip action="delete", it is the pathname to delete from the
 ZIP or JAR file.

### Attribute: `filter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: File filter applied to the action. For example, for the zip action,
 all the files in the source directory matching the filter are zipped.

### Attribute: `prefix`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: String added as a prefix to the ZIP or JAR entry.

Used only
 when cfzip action="zip".

### Attribute: `recurse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: Recurse the directory to be zipped, unzipped, or deleted,
 as specified by the cfzip parent tag.

### Attribute: `source`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Source directory or file.

Used only when cfzip action="zip".
 Specified file(s) are added to the ZIP or JAR file:
 If you specify source attribute for the cfzip tag, the
 cfzipparam source is relative to it.
 If you do not specify a source attribute for the cfzip
 tag, the cfzipparam source must be an absolute path.

## Limitations

- **Must be nested inside**: `cfzip`
- **Must not be nested inside**: *None*

