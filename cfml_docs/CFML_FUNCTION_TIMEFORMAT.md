# Function Name: `TimeFormat`

## Description
Formats a time value using US English time formatting conventions. If no mask is specified, returns a time value using the hh:mm tt format. For international time formatting, see LSTimeFormat.

## Return Type
`string`

## Syntax
```cfml
timeFormat(time [, mask])
```

## Arguments

### Argument: `time`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A date/time value or string to convert

### Argument: `mask`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `hh:mm tt`
- **Description**: Masking characters that determine the format.
 `h`,`hh`: Hours in 12 hour format
,`H`,`HH`: Hours in 24 hour format
 `m`,`mm`: Minutes
`s`,`ss`: Seconds
 `l`: Milliseconds
`t`: A or P
`tt`: AM or PM
`z`: Time zone in literal format, for example GMT
`Z`: Time zone in hours offset (RFC822), for example +0400
`X`,`XX`,`XXX`: Time zone in hours of offset in ISO 8601 format
`"short"`: `h:mm tt`
`"medium"`: `h:mm:ss tt`

## Limitations and Other Info

- **Related Functions**: `lsTimeFormat`, `dateFormat`
- **Coldfusion Support**: Notes: Member function is available in CF11+. The `z` `Z` and `X` masks were added in CF2016+
- **Lucee Support**: Notes: Member function is available in Lucee5+.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

