# Tag Name: `cfexchangefolder`

## Description
Lets you perform various actions on the mail folder, such as get folder information, find folders, or create, copy, modify, move, delete, and empty the contents of a folder.

## Syntax
```cfml
<cfexchangefolder action="" folderID="" connection="" name="">
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

### Attribute: `uid`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (getInfo/getExtendedInfo) UID that is used to identify the folder in which the actions are performed.

### Attribute: `name`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (getInfo/getExtendedInfo/findSubFolders) The name of the ColdFusion query variable that contains the returned information about the folder.

### Attribute: `folderID`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (getInfo/getExtendedInfo/findSubFolders/delete/modify/empty) UID that is used to identify the folder in which the actions are performed.

### Attribute: `folderPath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (getInfo/getExtendedInfo) Full path to the folder where the action has to be performed.

### Attribute: `pathDelimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `/`
- **Description**: (getInfo/getExtendedInfo) Lets you specify the delimiter that is used to separate the folders.

### Attribute: `result`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (create/copy/move) The name of a query variable that contains the result returned from the exchange server when one of the action is performed.

### Attribute: `destinationFolderID`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (copy/move) A case-sensitive Exchange UID value that uniquely identifies the destination folder.

### Attribute: `sourceFolderID`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (copy/move) The UID that is used to identify the folder from which you copy or move folders to the destination folder.

### Attribute: `deleteType`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `moveToDeletedItems`
- **Description**: (delete/move) - hardDelete: Removes a folder permanently from the Exchange server.
- softDelete: Moves a folder to the dumpster in Exchange server, if dumpster is enabled.
- moveToDeletedItems: Moves a folder to the deleted items folder.

### Attribute: `deleteSubFolders`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: (empty) If true, deletes the subfolder.

### Attribute: `folder`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (create/modify) A struct that contains the required information of the folder that has to be created or modified, such as display name and folder class.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

