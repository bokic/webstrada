# Tag Name: `cfregistry`

## Description
Reads, writes, and deletes keys and values in the system registry.
 Provides persistent storage of client variables.
 This tag is deprecated for the UNIX platform.
 Note: For this tag to execute, it must be enabled in the ColdFusion MX
 Administrator.

## Syntax
```cfml
<cfregistry action="getAll">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Action to perform

### Attribute: `branch`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of a registry branch.

### Attribute: `entry`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Registry value to access.
 Note: For key deletion this attribute is required.

### Attribute: `variable`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Variable into which to put value.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `String`
- **Description**: 

### Attribute: `sort`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Sorts query column data (case-insensitive). Sorts on Entry, Type,
 and Value columns as text. Specify a combination of columns from
 query output, in a comma-delimited list.
 For example: sort = "value desc, entry asc"
 * asc: ascending (a to z) sort order.
 * desc: descending (z to a) sort order.

### Attribute: `directory`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Absolute pathname of directory against which to perform
 action.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name for output record set.

### Attribute: `mode`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies only to UNIX and Linux. Permissions. Octal values
 of Unix chmod command. Assigned to owner, group, and
 other, respectively.

### Attribute: `newdirectory`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: New name for directory.

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Value data to set. If you omit this attribute, the cfregistry tag creates default value, as follows:
 * string: creates an empty string: "".
 * dWord: creates a value of 0 (zero).

### Attribute: `recurse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Whether ColdFusion performs the action on subdirectories.

### Attribute: `registryversion`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: No Help Available

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

