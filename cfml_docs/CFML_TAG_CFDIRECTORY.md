# Tag Name: `cfdirectory`

## Description
Allows you to list, create, delete or rename a directory in the server file system.

## Syntax
```cfml
<cfdirectory directory="." action="list|create|delete|rename">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `list`
- **Description**: Action to perform

### Attribute: `directory`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Absolute pathname of directory against which to perform action.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name for output record set.

### Attribute: `filter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Filter applied to returned names. For example: *.cfm
You can use a pipe ("|") delimiter to specify multiple filters. For example: *.cfm|*.cfc
Filter pattern matches are case-sensitive on UNIX and Linux.

### Attribute: `mode`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies only to UNIX and Linux. Permissions. Octal values of Unix chmod command. Assigned to owner, group, and other, respectively.

### Attribute: `sort`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `asc`
- **Description**: Query column(s) by which to sort directory listing.
 Delimited list of columns from query output.

### Attribute: `newdirectory`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: New name for directory.

### Attribute: `recurse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Whether ColdFusion performs the action on subdirectories.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `all`
- **Description**: 

### Attribute: `listinfo`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `all`
- **Description**: 

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

