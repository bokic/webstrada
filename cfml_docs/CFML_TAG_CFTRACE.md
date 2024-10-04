# Tag Name: `cftrace`

## Description
Displays and logs debugging data about the state of an
 application at the time the cftrace tag executes. Tracks
 runtime logic flow, variable values, and execution time.
 Displays output at the end of the request or in the debugging
 section at the end of the request;

 CFML logs cftrace output to the file logs\cftrace.log, in
 the CFML installation directory.

 Note: To permit this tag to execute, you must enable debugging
 in the CFML Administrator. Optionally, to report trace
 summaries, enable the Trace section.

## Syntax
```cfml
<cftrace>
```

## Attributes / Variants

### Attribute: `abort`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Calls cfabort tag when the tag is executed

### Attribute: `category`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: User-defined string for identifying trace groups

### Attribute: `inline`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Displays trace code in line on the page in the
 location of the cftrace tag, addition to the debugging
 information output.

### Attribute: `text`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: User-defined string, which can include simple variable,
 but not complex variables such as arrays. Outputs to cflog
 text attribute

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `information`
- **Description**: Corresponds to the cflog type attribute; displays an
 appropriate icon.

 * Information
 * Warning
 * Error
 * Fatal Information

### Attribute: `var`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a simple or complex variable to display.

 Useful for displaying a temporary value, or a value that
 does not display on any CFM page.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

