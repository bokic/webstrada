# Tag Name: `cfexchangeconversation`

## Description
Helps users organize and manage conversations from a Microsoft Exchange account.

## Syntax
```cfml
<cfexchangeconversation action="" connection="" folderID="" name="">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The action to take.

### Attribute: `connection`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of the connection to the Exchange server, as specified in the `cfexchangeconnection` tag.

### Attribute: `name`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (get) The name of the ColdFusion query variable that contains the returned conversation information.

### Attribute: `folderID`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A case-sensitive Exchange UID value that uniquely identifies the folder.

### Attribute: `UID`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A case-sensitive Exchange UID value that uniquely identifies the conversation.

### Attribute: `isRead`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (setReadState) Indicates the status of the conversation, if read or not.

### Attribute: `destinationFolderID`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (copy/move) A case-sensitive Exchange UID value that uniquely identifies the destination folder.

### Attribute: `deleteType`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `moveToDeletedItems`
- **Description**: (delete) - hardDelete: Removes a folder permanently from the store.
- softDelete: Removes a folder to the dumpster, if dumpster is enabled.
- moveToDeletedItems: Moves a folder to the deleted items folder.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

