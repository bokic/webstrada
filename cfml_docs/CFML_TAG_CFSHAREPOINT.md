# Tag Name: `cfsharepoint`

## Description
Invokes a feature that SharePoint exposes as a web service action, such as the Document Workspace
getdwsdata action.

## Syntax
```cfml
<cfsharepoint action="cancreatedwsurl">
```

## Attributes / Variants

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The password required to connect to the SharePoint server. Required if you do not specify a
login attribute.

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of a web service action. See Usage for the list of service actions you can specify.

### Attribute: `login`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A structure containing user, password, and domain login credentials to pass to the service. If you do not specify
domain ,password, and userNameattributes you must specify a login structure with domain, password, and userName entries.

### Attribute: `params`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A structure containing names and values of the parameters to pass to the service.

### Attribute: `domain`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The domain name required to connect to the SharePoint server. Required if you do not specify a login attribute.

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The user name required to connect to the SharePoint server. Required if you do not specify a
login attribute.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the result variable in which to put the data returned by the SharePoint service.

### Attribute: `wsdl`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Path to the service wsdl file. Required to invoke an action that is not in the list of supported actions. See Usage for details.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

