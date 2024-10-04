# Tag Name: `cflock`

## Description
Ensures the integrity of shared data. Instantiates the
 following kinds of locks:

 * Exclusive allows single-thread access to the CFML constructs
 * Read-only allows multiple requests to access CFML constructs

## Syntax
```cfml
<cflock name="lockName" timeout="3">
```

## Attributes / Variants

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Maximum length of time, in seconds, to wait to obtain a
 lock. If lock is obtained, tag execution continues.
 Otherwise, behavior depends on throwOnTimeout attribute
 value.

### Attribute: `scope`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lock scope. Mutually exclusive with the name attribute.
 Lock name. Only one request in the specified scope can
 execute the code within this tag (or within any other
 cflock tag with the same lock scope scope) at a time.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lock name. Mutually exclusive with the scope attribute.
 Only one request can execute the code within a cflock tag
 with a given name at a time. Cannot be an empty string.

### Attribute: `throwontimeout`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: How timeout conditions are handled.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `exclusive`
- **Description**: readOnly: lets more than one request read shared data.
 exclusive: lets one request read or write shared data.

### Attribute: `result`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `cflock`
- **Description**: Lucee4+ Specifies a name for the structure in which cflock returns the statusCode and ExecutionTime variables. Default variable is "cflock".

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

