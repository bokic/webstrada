# Tag Name: `cfmailparam`

## Description
Attaches a file or adds a header to an e-mail message. Can only
 be used in the cfmail tag. You can use more than one
 cfmailparam tag within a cfmail tag.

## Syntax
```cfml
<cfmailparam>
```

## Attributes / Variants

### Attribute: `file`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Attaches file to a message. Mutually exclusive with name
 attribute. The file is MIME encoded before sending.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The MIME media type of the part. Can be a can be valid MIME
 media type

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of header. Case-insensitive. Mutually exclusive with
 file attribute.
 The values listed are from rfc2822

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Value of the header.

### Attribute: `contentID`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The Identifier for the attached file. This ID should
 be globally unique and is used to identify the file in
 an IMG or other tag in the mail body that references
 the file content.

### Attribute: `disposition`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `attachment`
- **Description**: How the attached file is to be handled. Can be one
 of the following:
 - attachment: present the file as an attachment
 - inline: display the file contents in the message

### Attribute: `content`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lets you send the contents of a
ColdFusion variable as an attachment

### Attribute: `remove`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Tells ColdFusion to remove any attachments after successful mail delivery.

### Attribute: `filename`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF2016+ Lucee5.1.0.17+ The file name of the attachment as seen by the recipient.

## Limitations

- **Must be nested inside**: `cfmail`
- **Must not be nested inside**: *None*

