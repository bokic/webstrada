# Function Name: `WriteLog`

## Description
Writes a message to a log file.

## Return Type
`void`

## Syntax
```cfml
writeLog(text [, type] [, application] [, file] [, log] )
```

## Arguments

### Argument: `text`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Message to log. The date / time will be logged automatically for you.

### Argument: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Allowed Values**: `information`, `warning`, `error`, `fatal`
- **Description**: Type or severity of the log message

### Argument: `application`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: Logs the application name, if it is specified in Application.cfc or a cfapplication tag.

### Argument: `file`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The file name to log to. You cannot specify a directory path or file extension (extension will be `.log`). If the file does not exist, it is created automatically. The log file will be located in your CF server logs directory.

### Argument: `log`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Allowed Values**: `application`, `scheduler`
- **Description**: If you omit the file attribute, writes messages to standard
 log file. Ignored, if you specify file attribute.

 Application: writes to Application.log, normally used for
 application-specific messages.
 Scheduler: writes to Scheduler.log, normally used to log
 the execution of scheduled tasks.


## Limitations and Other Info
- **Limitation**: It is crucial to remember that the log attribute and the file attribute are mutually exclusive: If you provide a custom file name (e.g., file="payment_errors"), the engine completely ignores the log parameter and creates
- **Log location**: The log location is at `/var/log/webstrada/`
- **Related Functions**: `cflog`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**: Notes: In Lucee 6.2 or later the default log level is ERROR, so no messages will appear in the logfile unless you have specified type=error or type=fatal. You can change the default as discussed here: https://dev.lucee.org/t/application-log-defaults-to-error-in-6-2/14773
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.
