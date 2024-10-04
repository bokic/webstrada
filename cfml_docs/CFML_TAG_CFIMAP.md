# Tag Name: `cfimap`

## Description
Queries an IMAP server to retrieve and manage mails within multiple folders.

## Syntax
```cfml
<cfimap>
```

## Attributes / Variants

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the password for assessing the users e-mail account.

### Attribute: `secure`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies whether the IMAP server uses a Secure Sockets Layer.

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `getheaderonly`
- **Description**: * GetHeaderOnly - Returns the message header information for all retrieved mail.
* GetAll - Returns mail. The information includes the message header information, message text, and any attachments. Set the AttachmentPath attribute to retrieve attachments.
* Delete - Deletes messages from a folder.
* Open - Initiates an open session or connection with the IMAP server.
* Close - Terminates the open session or connection with the IMAP server.
* MarkRead - Marks all messages read from a folder.
* DeleteFolder - Deletes the identified folder.
* CreateFolder - Creates a folder in Inbox.
* RenameFolder - Renames an existing user-defined folder.
* ListAllFolders - Displays a list of all existing folders in the mailbox or under the folder name defined by the Folder attribute.
* MoveMail - Moves mail from one folder to another

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the number of seconds to wait before timing out connection to IMAP server. An error message is displayed when timeout occurs.

### Attribute: `messageNumber`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the message number or a comma delimited list of message numbers for retrieval, deletion, marking mail as read, or moving mails.
If you set an invalid message number or range, then it is ignored. If you have specified the UID attribute, then MessageNumber attribute is ignored.

### Attribute: `connection`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Required for the following actions: Open and Close - Specifies the variable name for the connection/session. For example, the e-mail login to an IMAP server can be used as the value for the connection. If the server attribute has an invalid IP address or invalid domain name, 
then the connection fails and ColdFusion returns an error message.

### Attribute: `newFolder`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the name of the destination folder where all mail move.

### Attribute: `uid`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the unique ID or a comma-delimited list of Uids to retrieve or delete. If you set invalid Uids, then they are ignored.

### Attribute: `folder`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For mail actions: Specifies the folder name where messages are searched, retrieved, moved, or deleted. If folder name is invalid, ColdFusion defaults to INBOX.
For folder actions: Specifies the folder name that is deleted (DeleteFolder) or created (CreateFolder) or renamed (RenameFolder).

### Attribute: `port`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the IMAP port number. Use 993 for secured connections.

### Attribute: `stoponerror`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies whether or not to ignore the exceptions for this operation. When the value is true, it stops processing, displays an appropriate error.

### Attribute: `generateUniqueFileNames`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Ensures that unique file names are generated for each attachment file. 
The goal is to avoid name conflicts for attachments that have the same filename.

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the number of rows to be marked as read, deleted, or moved across folders. When the value is 1, it signals the row determined by StartRow. Any incremental value marks rows starting from the StartRow.
If you have specified the UID or MessageNumber attribute, then MaxRows is ignored.

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the user name. Typically, the user name is same the e-mail login.

### Attribute: `startRow`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Defines the first row number for reading or deleting. If you have specified the UID or MessageNumber attribute, then StartRow is ignored. You can also specify StartRow for moving mails.

### Attribute: `attachmentpath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Required for GetAll action - Specifies the name of the folder where ColdFusion retrieves attachments. If this folder does not exist, ColdFusion creates it.

### Attribute: `server`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the IMAP server identifier. You can assign a host name or an IP address as the IMAP server identifier. For example, imap.gmail.com.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the name for the query object that contains the retrieved message information.

### Attribute: `recurse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies whether ColdFusion runs the CFIMAP command in subfolders.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

