# Tag Name: `cfexchangecalendar`

## Description
Creates, deletes, modifies, gets, and responds to Microsoft Exchange calendar events, and gets calendar event attachments.

## Syntax
```cfml
<cfexchangecalendar action="create" event="" name="" responsetype="accept" uid="">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The action to take. Must be one of the following: create, delete, get, getAttachments, modify, respond (required)

### Attribute: `attachmentpath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The file path of the destination directory.
 If the directory does not exist, ColdFusion creates it.
 If you omit this attribute, ColdFusion does not save any attachments.
 If you specify a relative path, the path root is the ColdFusion temporary directory, which is returned by the GetTempDirectory function. (optional)

### Attribute: `connection`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the connection to the Exchange server, as specified in the cfexchangeconnection tag.
 If you omit this attribute, you must create a temporary connection by specifying cfexchangeconnection tag connection attributes in the cfexchangecalendar tag. (optional)

### Attribute: `event`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A reference to the structure that contains the event properties to be set or changed and their values.
 You must specify this attribute in number signs (#). (required)

### Attribute: `generateUniquefilenames`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value specifying whether to generate unique file names if multiple attachments have the same file names.
 Case "yes": 3x myfile.txt -> myfile.txt, myfile1.txt, and myfile2.txt. (optional, default=no)

### Attribute: `message`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The text of an optional message to send in the response or deletion notification. (optional)

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of the ColdFusion query variable that will contain the retrieved events or information about the attachments that were retrieved. (required)

### Attribute: `notify`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Boolean value specifying whether to notify others of the changes made to the event (optional)

### Attribute: `responseType`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: (respond) Must be one of the following: accept, decline, tentative (required)

### Attribute: `result`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a variable that will contain the UID of the event that is created.
 You use the UID value in the uid attribute other actions to identify the event to be acted on. (optional)

### Attribute: `uid`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Case-sensitive Exchange UID value or values that uniquely identify the event or events
 on which to perform the action.
 For the delete action, this attribute can be a comma delimited list of UID values.
 The getAttachments, modify, and respond actions allow only a single UID value. (required)

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

