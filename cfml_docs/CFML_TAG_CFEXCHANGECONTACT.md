# Tag Name: `cfexchangecontact`

## Description
Creates, deletes, modifies, and gets Microsoft Exchange contact records, and gets contact record attachments.

## Syntax
```cfml
<cfexchangecontact action="create" contact="" name="" uid="">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The action to take. Must be one of the following: create, delete, get, getAttachments, modify (required)

### Attribute: `attachmentPath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The absolute file path of the directory in which to put the attachments.
 If the directory does not exist, ColdFusion creates it.
 If you omit this attribute, ColdFusion does not save any attachments. (optional)

### Attribute: `connection`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the connection to the Exchange server, as specified in the cfexchangeconnection tag.
 If you omit this attribute, you must create a temporary connection by specifying
 cfexchangeconnection tag connection attributes in the cfexchangecontact tag. (optional)

### Attribute: `contact`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A reference to the structure that contains the contact properties to be set or changed and their values.
 You must specify this attribute in number signs (#).
 For more information on the event structure, see the Usage section. (required)

### Attribute: `generateUniqueFilenames`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value specifying whether to generate unique file names if multiple attachments have the same file names.
 Case "yes": 3x myfile.txt -> myfile.txt, myfile1.txt, and myfile2.txt. (optional, default=no)

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of the ColdFusion query variable that will contain the retrieved events or
 information about the attachments that were retrieved. (required)

### Attribute: `result`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a variable that will contain the UID of the contact that is created.
 You use this value in the uid attribute other actions to identify the contact to be acted on. (optional)

### Attribute: `uid`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A case-sensitive Exchange UID value that uniquely identifies the contacts on which to perform the action.
 For the delete action, this attribute can be a comma delimited list of UID values.
 The getAttachments and modify action allow only a single UID value. (required)

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

