# Tag Name: `cfexchangeconnection`

## Description
Opens or closes a persistent connection to an Microsoft Exchange server.
 You must have a persistent or temporary connection to use the cfexchangecalendar,
 cfexchangecontact, cfexchangemail, and cfexchangetask tags to get or change
 information on the Exchange server.

## Syntax
```cfml
<cfexchangeconnection action="open" connection="" server="" username="">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The action to take. Must be open or close. (required)

### Attribute: `connection`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of the connection. You specify this ID when you close the connection
 and in tags such as cfexchangemail. (required)

### Attribute: `mailboxName`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The ID of the Exchange mailbox to use.
 Specify this attribute to access a mailbox whose owner has delegated access
 rights to the account specified in the username attribute. (optional)

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (open) The users password for accessing the Exchange server. (optional)

### Attribute: `port`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The port on the server connect to, most commonly port 80. (optional)

### Attribute: `protocol`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The protocol to use for the connection. Valid values are http and https. (optional)

### Attribute: `proxyHost`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The URL or IP address of the proxy host required for access to the network. (optional)

### Attribute: `proxyPort`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The port on the proxy server to connect to, most commonly port 80. (optional)

### Attribute: `server`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The IP address or URL of the server that is providing access to Exchange. (required)

### Attribute: `username`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The Exchange user ID (required)

### Attribute: `folder`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The forward slash (/) delimited path from the root of the mailbox to the folder for which to get subfolders.

### Attribute: `recurse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: * true: get information on the immediate subfolders of the specified folder only.
 * false: get information on all levels of subfolders of the specified folder.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the ColdFusion query variable that contains information about the subfolders.

### Attribute: `exchangeServerLanguage`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The language of the Exchange server. Default is English.

### Attribute: `formBasedAuthentication`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value that specifies whether to display a login form and use form based authentication when making the connection.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

