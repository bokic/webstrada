# Tag Name: `cfexchangetask`

## Description
Creates, deletes, modifies, and gets Microsoft Exchange tasks, and gets task attachments.

## Syntax
```cfml
<cfexchangetask task="" name="" uid="">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The action to take. Must be one of the following: create, delete, get, getAttachments, modify (optional)

### Attribute: `attachmentPath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The file path of the directory in which to put the attachments.
 If the directory does not exist, ColdFusion creates it.
 If you omit this attribute, ColdFusion does not save any attachments.
 If you specify a relative path, the path root is the ColdFusion temporary directory, which is returned
 by the GetTempDirectory function. (optional)

### Attribute: `connection`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the connection to the Exchange server, as specified in the cfexchangeconnection tag.
 If you omit this attribute, and you specify cfexchangeconnection tag attributes in this tag,
 ColdFusion creates a temporary connection. (optional)

### Attribute: `task`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A reference to the structure that contains the task properties to be set or changed and their values.
 You must specify this attribute in number signs (#).
 For more information on the event structure, see the Usage section. (required)

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of the ColdFusion query variable that will contain the returned mail messages or information
 about the attachments that were retrieved. (required)

### Attribute: `results`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (create) The name of a variable that will contain the UID of the task that is created.
 You use this value in the uid attribute of other actions to identify the task to be acted on. (optional)

### Attribute: `uid`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A case-sensitive Exchange UID value that uniquely identifies the tasks on which to perform the action.
 For the delete action, this attribute can be a comma delimited list of UID values.
 The getAttachments and modify action allow only a single UID value. (optional)

### Attribute: `result`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a variable that contains the UID of the task that is created. You use this value in the uid attribute of other actions to identify the task to be acted on.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

