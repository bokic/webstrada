# Tag Name: `cfNTauthenticate`

## Description
Authenticates a user name and password against the
 NT domain on which ColdFusion server is running,
 and optionally retrieves the user's groups.

## Syntax
```cfml
<cfntauthenticate username="" password="" domain="">
```

## Attributes / Variants

### Attribute: `username`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: User's login name.

### Attribute: `password`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: User's login name.

### Attribute: `domain`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Domain against which to authenticate the user. The
 ColdFusion J2EE server must be running on this domain.

### Attribute: `result`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: `cfntauthenticate`
- **Description**: Name of the variable in which to return the results.
 Default: cfntauthenticate

### Attribute: `listgroups`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Boolean value specifying whether to Include a
 comma-delimited list of the user's groups in the
 result structure.
 Default: false

### Attribute: `throwonerror`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Boolean value specifying whether to throw an
 exception if the validation fails. If this attribute is true,
 ColdFusion throws an error if the user name or password is
 invalid; the application must handle such errors in a
 try/catch block or ColdFusion error handler page.
 Default: false

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

