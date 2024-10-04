# Function Name: `CreateTimeSpan`

## Description
Returns a value that defines a time period, represented by a numeric (double) where 1 equals 1 day. You can add or subtract it from other date/time objects and use it with the cachedWithin attribute of cfquery.

## Return Type
`numeric`

## Syntax
```cfml
createTimespan(days, hours, minutes, seconds)
```

## Arguments

### Argument: `days`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of days ranging from 0 to 32768

### Argument: `hours`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of hours

### Argument: `minutes`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of minutes

### Argument: `seconds`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of seconds

## Limitations and Other Info

- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

