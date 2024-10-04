# Tag Name: `cfexecute`

## Description
Executes a CFML developer-specified process on a server computer.

## Syntax
```cfml
<cfexecute name="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Absolute path of the application to execute.

 On Windows, you must specify an extension; for example,
 C:\myapp.exe.

### Attribute: `arguments`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Command-line variables passed to application. If specified
 as string, it is processed as follows:
 * Windows: passed to process control subsystem for parsing.
 * UNIX: tokenized into an array of arguments. The default
 token separator is a space; you can delimit arguments
 that have embedded spaces with double quotation marks.
 If passed as array, it is processed as follows:
 * Windows: elements are concatenated into a string of
 tokens, separated by spaces. Passed to process control
 subsystem for parsing.
 * UNIX: elements are copied into an array of exec()
 arguments

### Attribute: `outputfile`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: File to which to direct program output. If no outputfile or
 variable attribute is specified, output is displayed on
 the page from which it was called.

 If not an absolute path (starting a with a drive letter and
 a colon, or a forward or backward slash), it is relative
 to the CFML temporary directory, which is returned
 by the GetTempDirectory function.

### Attribute: `variable`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Variable in which to put program output. If no outputfile
 or variable attribute is specified, output is displayed on
 page from which it was called.

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Length of time, in seconds, that CFML waits for
 output from the spawned program.

### Attribute: `errorVariable`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a variable in which to save the error stream output.

### Attribute: `errorFile`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The pathname of a file in which to save the error stream output. If not an
absolute path (starting a with a drive letter and a colon, or a forward or backward slash), it is
relative to the ColdFusion temporary directory, which is returned by the GetTempDirectory
function.

### Attribute: `terminateOnTimeout`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Lucee4.5+ Terminate the process after the specified timeout is reached. Ignored if timeout is not set or is 0.

### Attribute: `directory`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee5.3.8+ The working directory in which to execute the command

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

