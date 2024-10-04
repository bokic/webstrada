# Tag Name: `cfapplication`

## Description
Defines the scope of a CFML application and allows you to set various application specific settings. Consider using Application.cfc instead of Application.cfm files.

## Syntax
```cfml
<cfapplication>
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of application. Up to 64 characters

### Attribute: `loginstorage`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `cookie`
- **Description**: cookie: store login information in the Cookie scope.
 session: store login information in the Session scope.

### Attribute: `clientmanagement`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: enables client variables

### Attribute: `clientstorage`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `registry`
- **Description**: How client variables are stored
 * datasource_name: in ODBC or native data source.
 You must create storage repository in the
 Administrator.
 * registry: in the system registry.
 * cookie: on client computer in a cookie. Scalable.
 If client disables cookies in the browser, client
 variables do not work

### Attribute: `setclientcookies`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: No: CFML does not automatically send CFID and CFTOKEN
 cookies to client browser; you must manually code CFID and
 CFTOKEN on the URL for every page that uses Session or
 Client variables

### Attribute: `sessionmanagement`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: enables session variables

### Attribute: `sessiontimeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lifespan of session variables. CreateTimeSpan function and
 values in days, hours, minutes, and seconds, separated by
 commas

### Attribute: `applicationtimeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lifespan of application variables. CreateTimeSpan function
 and values in days, hours, minutes, and seconds, separated
 by commas.

### Attribute: `setdomaincookies`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: Sets CFID and CFTOKEN cookies for a domain (not a host)
 Required, for applications running on clusters.

### Attribute: `scriptprotect`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies whether to protect variables from cross-site scripting attacks.
 - none: do not protect variables
 - all: protect Form, URL, CGI, and Cookie variables
 - comma-delimited list of ColdFusion scopes: protect variables in the specified scopes

### Attribute: `securejsonprefix`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The security prefix to put in front of the value that a ColdFusion function returns in JSON-format 
				in response to a remote call if the secureJSON setting is true.

### Attribute: `securejson`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value that specifies whether to add a security prefix in front of any value that a ColdFusion function returns in JSON-format
				 in response to a remote call.

### Attribute: `serverSideFormValidation`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Enable/Disable ColdFusion‚ server side validation on CFFORM.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

