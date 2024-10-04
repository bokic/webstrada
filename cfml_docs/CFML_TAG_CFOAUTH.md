# Tag Name: `cfoauth`

## Description
The <oauth> tag allows you to easily integrate third-party Oauth 2 authentication provider in your application. This tag currently supports Facebook and Google authentication. Also, this tag supports Oauth providers that support the Oauth 2 protocols. For instance, Microsoft and GitHub. 

If type is not Facebook or Google then use access token endpoint and other attributes.

## Syntax
```cfml
<cfoauth>
```

## Attributes / Variants

### Attribute: `Type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Currently supported values are Facebook and Google. Implicitly supports the authentication workflow of Facebook and Google.

### Attribute: `clientid`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Unique ID generated while registering your application with the Oauth provider.

### Attribute: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Scopes are the permissions that a developer seeks from the users. These are usually comma separated values of permissions.
Refer to the Oauth provider's documentations for more information.
For example, after Facebook authentication, if a developer wants to access an email address and then the friend lists of a user, the developer will use:
scope=email,read_friendlists.
Note: The scope name varies for different Oauth providers.

### Attribute: `state`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The state variable is used to pass back any information to your web application after the authentication and redirection are completed. Any value passed to this attribute is returned to the web application after authentication. This is useful for CSRF (Cross-site request forgery) protection. You can use ColdFusion’s security-related CSRF functions for this attribute.

### Attribute: `authendpoint`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If type is not specified, this will be used as endpoint URL to be invoked for user authentication.

### Attribute: `accesstokenendpoint`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**:  If type is not specified this will be used as end point URL to be invoked for app authentication.

### Attribute: `secretkey`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Parameter is the App Secret as displayed in your social media app's settings.

### Attribute: `result`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A struct which will have login info of the user including login success/failure, failure reason, user name, user id.

### Attribute: `redirecturi`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: This will default to the URL which is executing the code. So if in oauth settings user has given app URL as : http://domainname/appname And the file executing the code is : http://domainname/appname/login.cfm The redirect URI will be : http://domainname/appname/login.cfm

### Attribute: `urlparams`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Extra options which will be passed as URL query string to authendpoint.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

