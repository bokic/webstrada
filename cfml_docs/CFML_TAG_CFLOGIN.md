# Tag Name: `cflogin`

## Description
A container for user login and authentication code. CFML
 runs the code in this tag if a user is not already logged in.
 You put code in the tag that authenticates the user and
 identifies the user with a set of roles. Used with cfloginuser
 tag.

## Syntax
```cfml
<cflogin>
```

## Attributes / Variants

### Attribute: `idletimeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Time interval with no keyboard activity after which
 CFML logs the user off. Seconds.

### Attribute: `applicationtoken`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Unique application identifier. Limits the login validity to
 one application, as specified by the cfapplication tag.

### Attribute: `cookiedomain`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Domain of the cookie that is used to mark a user as logged
 in. Use this attribute to enable a user login cookie to
 work with multiple clustered servers in the same domain.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

