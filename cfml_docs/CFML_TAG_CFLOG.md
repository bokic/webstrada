# Tag Name: `cflog`

## Description
Writes a message to a log file.

## Syntax
```cfml
<cflog text="">
```

## Attributes / Variants

### Attribute: `text`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Message text to log.

### Attribute: `log`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If you omit the file attribute, writes messages to standard
 log file. Ignored, if you specify file attribute.

 Application: writes to Application.log, normally used for
 application-specific messages.
 Scheduler: writes to Scheduler.log, normally used to log
 the execution of scheduled tasks.

### Attribute: `file`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Message file. Specify only the main part of the filename.
 For example, to log to the Testing.log file, specify
 "Testing".

 The file must be located in the default log directory. You
 cannot specify a directory path. If the file does not
 exist, it is created automatically, with the suffix .log.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Type (severity) of the message

### Attribute: `application`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: log application name, if it is specified in a cfapplication
 tag.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

