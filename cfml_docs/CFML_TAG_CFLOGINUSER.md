# Tag Name: `cfloginuser`

## Description
Identifies an authenticated user to CFML. Specifies the
 user ID and roles. Used within a cflogin tag.

## Syntax
```cfml
<cfloginuser name="" password="" roles="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A username.

### Attribute: `password`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A user password.

### Attribute: `roles`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A comma-delimited list of role identifiers.

 CFML processes spaces in a list element as part of
 the element.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

