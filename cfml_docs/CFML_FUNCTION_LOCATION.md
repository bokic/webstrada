# Function Name: `Location`

## Description
Stops execution of the current page and redirects to the given URL.

## Return Type
`void`

## Syntax
```cfml
location(url [, addtoken] [, statuscode])
```

## Arguments

### Argument: `url`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: URL of web page to open.

### Argument: `addtoken`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: clientManagement must be enabled (see cfapplication).

### Argument: `statuscode`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `302`
- **Description**: The HTTP status code

## Limitations and Other Info

- **Related Functions**: `cfscript`, `cflocation`
- **Coldfusion Support**: Minimum version: `9`. Notes: Implemented as both a function and an alias to the cflocation tag in script, so named parameters can be used prior to CF2018+. CF11+ `addtoken` default value is `false` when Secure Profile is enabled
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

