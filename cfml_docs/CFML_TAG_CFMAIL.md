# Tag Name: `cfmail`

## Description
Sends an email message that optionally contains query output, using an SMTP server.

## Syntax
```cfml
<cfmail to="" from="" subject="">
```

## Attributes / Variants

### Attribute: `async`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ Specifies the mail is sent asynchronously by the Lucee Task manager (with multiple tries), if set to false the mail is sent in the same thread that executes the request, which is useful for troubleshooting because you get an error message if there is one. This setting overrides the setting with the same name in the Lucee Administrator. This attribute replaces the old 'spoolenable' attribute which is still supported as an alias.

### Attribute: `bcc`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Email address(es) to which to copy the message, without listing them in the message header.

### Attribute: `cc`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Email address(es) to which to copy the message

### Attribute: `charset`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The character encoding in which the text part is encoded.

### Attribute: `debug`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: true: sends debugging output to standard output. By default, if the console window is unavailable, ColdFusion sends output to cf_root\runtime\logs\coldfusion-out.log on server configurations. On J2EE configurations, with JRun, the default location is jrun_home/logs/servername-out.log.
false: does not generate debugging output.

### Attribute: `encrypt`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF11+ Toggles email message encryption

### Attribute: `encryptionalgorithm`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF11+ Algorithm to use when encrypt=true
Encryption support is provided through S/MIME.

### Attribute: `failto`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Email address to which mailing systems should send delivery failure notifications. Sets the mail envelope reverse-path value.

### Attribute: `from`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Message sender email address.

### Attribute: `group`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Query column to use when you group sets of records to send as a message. For example, to send a set of billing statements to a customer, group on "Customer_ID." Case-sensitive. Eliminates adjacent duplicates when data is sorted by the specified field.

### Attribute: `groupcasesensitive`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to consider case when using the group attribute. To group on case-sensitive records, set this attribute to Yes.

### Attribute: `keyalias`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Alias of the key with which the certificate and private key is stored in keystore. If it is not specified then the first entry in the keystore will be picked up.

### Attribute: `keypassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Password with which the private key is stored. If it is not specified, keystorepassword will be used as keypassword as well.

### Attribute: `keystore`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Keystore containing the private key and certificate. The supported type is JKS (java key store) and pkcs12

### Attribute: `keystorepassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Password of the keystore

### Attribute: `mailerid`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Mailer ID to be passed in X-Mailer SMTP header, which identifies the mailer application.

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum number of messages to send when looping over a query.

### Attribute: `mimeattach`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Path of file to attach to message. Attached file is MIME-encoded. CFML attempts to determine the MIME type of the file; use the cfmailparam tag to send an attachment and specify the MIME type.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A password to send to SMTP servers that require authentication. Requires a username attribute.

### Attribute: `port`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: TCP/IP port on which SMTP server listens for requests (normally 25). A value here overrides the Administrator.

### Attribute: `priority`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `normal`
- **Description**: The message priority level. Can be an integer in the range 1-5; 1 represents the highest priority, or one of the following string values, which correspond to the numeric values

### Attribute: `proxyserver`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ Host name or IP address of a proxy server.

### Attribute: `proxyport`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ The port number on the proxy server from which the object is requested. Default is 80. When used with resolveURL, the URLs of retrieved documents that specify a port number are automatically resolved to preserve links in the retrieved document.

### Attribute: `proxyuser`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ When required by a proxy server, a valid username.

### Attribute: `proxypassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ When required by a proxy server, a valid password.

### Attribute: `query`
- **Type**: `query`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of cfquery from which to draw data for message(s). Use this attribute to send more than one message, or to send query results within a message.

### Attribute: `recipientcert`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF11+ Path to the public key certificate of the recipient.

### Attribute: `remove`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Tells ColdFusion to remove any attachments after successful mail delivery.

### Attribute: `replyto`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Email address(es) to which the recipient is directed to send replies.

### Attribute: `sendtime`
- **Type**: `date`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+ Set a future date time to send an email in the future via the spooler.

### Attribute: `server`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: SMTP server address, or (Enterprise edition only) a comma-delimited list of server addresses, to use for sending messages. At least one server must be specified here or in the CFML MX Administrator. A value here overrides the Administrator. A value that includes a port specification overrides the port attribute.

### Attribute: `sign`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Mail will be signed when set to true

### Attribute: `spoolenable`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies whether to spool mail or always send it Immediately. Overrides the CFML MX Administrator Spool mail messages to disk for delivery setting.

### Attribute: `startrow`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: Row in a query to start from.

### Attribute: `subject`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Message subject. Can be dynamically generated.

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Number of seconds to wait before timing out connection to SMTP server. A value here overrides the Administrator.

### Attribute: `to`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Message recipient email addresses. To specify multiple addresses, separate the addresses with commas.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `text/plain`
- **Description**: The MIME media type of the part. Can be a can be valid MIME media type

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A user name to send to SMTP servers that require authentication. Requires a password attribute

### Attribute: `usessl`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to use Secure Sockets Layer.

### Attribute: `usetls`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to use Transport Level Security.

### Attribute: `wraptext`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the maximum line length, in characters of the mail text. If a line has more than the specified number of characters, replaces the last white space character, such as a tab or space, preceding the specified position with a line break. If there are no white space characters, inserts a line break at the specified position. A common value for this attribute is 72.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: `cfmail`

