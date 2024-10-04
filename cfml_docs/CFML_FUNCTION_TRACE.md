# Function Name: `Trace`

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
 summaries, enable the trace section.

## Return Type
`void`

## Syntax
```cfml
trace([var] [, text] [, type] [, category] [, inline] [, abort])
```

## Arguments

### Argument: `var`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a simple or complex variable to display.

 Useful for displaying a temporary value, or a value that
 does not display on any CFM page.

### Argument: `text`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: User-defined string, which can include simple variable,
 but not complex variables such as arrays. Outputs to cflog
 text attribute

### Argument: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `information`
- **Description**: Corresponds to the cflog type attribute; displays an
 appropriate icon.

 * Information
 * Warning
 * Error
 * Fatal Information

### Argument: `category`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: User-defined string for identifying trace groups

### Argument: `inline`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Displays trace code in line on the page in the
 location of the cftrace tag, addition to the debugging
 information output.

### Argument: `abort`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Calls cfabort tag when the tag is executed

## Limitations and Other Info

- **Related Functions**: `cftrace`
- **Coldfusion Support**: Minimum version: `9`. Notes: As of CF11 parameters must be comma separated.
- **Lucee Support**: Notes: On Lucee the caller scope must be passed in as the first argument.
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

