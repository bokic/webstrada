# Tag Name: `cfschedule`

## Description
Provides a programmatic interface to the CFML scheduling engine. Can run a CFML page at scheduled intervals, with the option to write the page output to a static HTML page. This feature enables you to schedule pages that publish data, such as reports, without waiting while a database transaction is performed to populate the page.

## Syntax
```cfml
<cfschedule action="delete" task="">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: delete: deletes the specified task
 update: updates an existing task or creates a new task, if one with the name specified by the task attribute does not exist
 run: executes the specified task
 pause: Pauses the specified task.
 resume: Continues executing the specified task.
 list: Lists all the scheduled tasks.
 pauseall: CF10+ Pauses all scheduled tasks.
 resumeall: CF10+ Resume all scheduled tasks for a particular application.
 create: CF2018u2+ Create a fresh task. If a task already exists, an error is thrown.
 modify: CF2018u2+ Modifies an existing task while retaining its old values.

### Attribute: `task`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the task. Not required if action attribute is set to list, otherwise it is required.

### Attribute: `operation`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `HTTPRequest`
- **Description**: Operation that the scheduler performs. Must be HTTPRequest.

### Attribute: `file`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the file in which to store the published output of the scheduled task.

### Attribute: `path`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Path to the directory in which to put the published file.
NOTE: This is Required if `publish` is "Yes".

### Attribute: `startdate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Date on which to first run the scheduled task.

### Attribute: `starttime`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Time at which to run the scheduled of task starts.

### Attribute: `URL`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: URL of the page to execute.

### Attribute: `port`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `80`
- **Description**: Port to use on the server that is specified by the url parameter. If resolveURL = "yes", retrieved document URLs that specify a port number are automatically resolved, to preserve links in the retrieved document. A port value in the url attribute overrides this value.

### Attribute: `publish`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: save the result to a file
 No: does not

### Attribute: `endDate`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Date when scheduled task ends.

### Attribute: `endTime`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Time when scheduled task ends (seconds).

### Attribute: `interval`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Interval at which task is scheduled.
 * number of seconds (minimum is 60)
 * once
 * daily
 * weekly
 * monthly

### Attribute: `requesttimeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Deprecated as of CF11+, Removed in CF2018 Used to extend the default timeout period.

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Username, if URL is protected.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Password, if URL is protected.

### Attribute: `proxyserver`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Host name or IP address of a proxy server.

### Attribute: `proxyport`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `80`
- **Description**: Port number to use on the proxy server.

### Attribute: `proxyuser`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: User name to provide to the proxy server.

### Attribute: `proxypassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Password to provide to the proxy server.

### Attribute: `resolveurl`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: resolve links in the output page to absolute references
 No: does not

### Attribute: `group`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `default`
- **Description**: CF11+ The group to which the scheduled task belongs.

### Attribute: `mode`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `server`
- **Description**: CF10+ If the task is server-specific or application specific.

### Attribute: `result (CF10+)/returnvariable (lucee)`
- **Type**: `query`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name for the query in which cfschedule returns the result variables.
NOTE: Required for `action`="list"

### Attribute: `eventHandler`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ A CFC file which implements CFIDE.scheduler.ITaskEventHandler and is invoked for events while running the task.
 Note: CF 2018 Enterprise Required; not supported in Standard Edition

### Attribute: `onException`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `invokeHandler, if eventHandler is specified`
- **Description**: CF10+ Specify the action to take if a task results in error.
 Note: CF 2018 Enterprise Required; not supported in Standard Edition

### Attribute: `onComplete`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `invokeHandler`
- **Description**: CF10+ The action or task to perform after completion of the current task. Can be used to chain dependent tasks by executing a task after this task completes.
 Note: CF 2018 Enterprise Required; not supported in Standard Edition

### Attribute: `onMisfire`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `invokeHandler if eventHandler is specified`
- **Description**: CF10+ Specify what to do if a task misfires. If unspecified, then no action is taken.
 Note: CF 2018 Enterprise Required; not supported in Standard Edition

### Attribute: `cronTime`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ Schedule the task time in quartz cron expression format (6 or 7 space-seperated values). Format is: second, minute, hour, day of month, month, day of week, year. Second value is required, as are the rest, but year is optional.

### Attribute: `repeat`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `-1`
- **Description**: CF10+ Specify the number of times a given schedule has to repeat.

### Attribute: `retryCount`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `3`
- **Description**: CF10+ Specify the number of times to retry the task if the task fails. Must be between 0 and 3, inclusive.

### Attribute: `priority`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `5`
- **Description**: CF10+ Set the priority of this task.

### Attribute: `exclude`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ Comma-separated list of dates or date range on which to not execute the scheduled task.

### Attribute: `cluster`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `no`
- **Description**: CF10+ If yes, the task can be executed in a cluster setup.
 Note: CF 2018 Enterprise Required; not supported in Standard Edition

### Attribute: `overwrite`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: CF10+ Specify whether to overwrite the output files on task execution (if true) or create new output files (if false).

### Attribute: `unique`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: lucee4.5+ If true, the scheduled task is only executed once at time. If a task is still running from previous round no new task is started.

### Attribute: `autodelete`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: lucee4.5+ If set to true, the scheduled task will be deleted when there is no possible future execution.

### Attribute: `readonly`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: lucee4.5+ If true, the scheduled task can not be modified or deleted in the Lucee Administrator.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

