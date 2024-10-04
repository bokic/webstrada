# Tag Name: `cfpop`

## Description
Retrieves or deletes e-mail messages from a POP mail server.

## Syntax
```cfml
<cfpop server="">
```

## Attributes / Variants

### Attribute: `server`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: POP server identifier:
 A host name; for example, "biff.upperlip.com"
 An IP address; for example, "192.1.2.225"

### Attribute: `port`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: POP port

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides username.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides password

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: getHeaderOnly: returns message header information only
 getAll: returns message header information, message text,
 and attachments if attachmentPath is specified
 delete: deletes messages on POP server
 markRead: marks the message as read

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name for query object that contains the retrieved message
 information.

### Attribute: `messagenumber`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Message number or comma-delimited list of message numbers
 to get or delete. Invalid message numbers are ignored.
 Ignored if uid is specified.

### Attribute: `uid`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: UID or a comma-delimited list of UIDs to get or delete.
 Invalid UIDs are ignored.

### Attribute: `attachmentpath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If action="getAll", specifies a directory in which to save
 any attachments. If the directory does not exist,
 CFML creates it.

 If you omit this attribute, CFML does not save any
 attachments. If you specify a relative path, the path root
 is the CFML temporary directory, which is returned by
 the GetTempDirectory function.

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum time, in seconds, to wait for mail processing

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Number of messages to return or delete, starting with the
 number in startRow. Ignored if messageNumber or uid is
 specified.

### Attribute: `startrow`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: First row number to get or delete. Ignored if messageNumber
 or uid is specified.

### Attribute: `generateUniqueFileNames`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Yes: Generate unique filenames for files attached to an
 e-mail message, to avoid naming conflicts when files are
 saved

### Attribute: `secure`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF10+ Enables SSL for pop requests.

### Attribute: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF11+ The value of the uid attribute can be a comma-separated
 list of UIDs. If the delimiter attribute is specified, the value
 of delimiter will be used as a delimiter instead of comma.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

