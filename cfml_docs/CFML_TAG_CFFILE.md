# Tag Name: `cffile`

## Description
Manages interactions with server files.
 Different combinations cause different attributes to be
 required.

## Syntax
```cfml
<cffile action="read">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Type of file manipulation that the tag performs.

### Attribute: `file`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Pathname of the file.

### Attribute: `mode`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies only to UNIX and Linux. Permissions. Octal values
 of Unix chmod command. Assigned to owner, group, and
 other, respectively.

### Attribute: `output`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: String to add to the file

### Attribute: `addnewline`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Yes: appends newline character to text written to file

### Attribute: `attributes`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies to Windows. A comma-delimited list of attributes
 to set on the file.

 If omitted, the file's attributes are maintained.

### Attribute: `charset`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The character encoding in which the file contents is
 encoded.

 For more information on character encodings, see:
 www.w3.org/International/O-charset.html.

### Attribute: `source`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Pathname of the file (during copy).

### Attribute: `destination`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Pathname of a directory or file on web server
 (during copy).

### Attribute: `variable`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of variable to contain contents of text file.

### Attribute: `filefield`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of form field used to select the file.

 Do not use pound signs (#) to specify the field name.

### Attribute: `nameconflict`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Action to take if filename is the same as that of a file
 in the directory.

### Attribute: `accept`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Limits the MIME types to accept. Comma-delimited list. For
 example, to permit JPG and Microsoft Word file uploads:

 accept = "image/jpg, application/msword"

### Attribute: `result`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: `cffile`
- **Description**: Allows you to specify a name for the variable in which cffile
 returns the result (or status) parameters. If you do not specify
 a value for this attribute, cffile uses the prefix "cffile".

### Attribute: `fixnewline`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: * Yes: changes embedded line-ending characters in string
 variables to operating-system specific line endings
 * No: (default) do not change embedded line-ending
 characters in string variables.

### Attribute: `cachedwithin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee5+ Timespan, using the CreateTimeSpan function. If original
 file date falls within the time span, cached file data is
 used. CreateTimeSpan defines a period from the present, back.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

