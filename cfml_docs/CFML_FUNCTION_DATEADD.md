# Function Name: `DateAdd`

## Description
Adds units of time to a date.

## Return Type
`date`

## Syntax
```cfml
dateAdd(datepart, number, date)
```

## Arguments

### Argument: `datepart`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: `yyyy` - Year
`q` - Quarter
`m` - Month
`y` - Day of year
`d` - Day
`w` - Week day
`ww` - Week
`h` - Hour
`n` - Minute
`s` - Second
`l` - Millisecond

### Argument: `number`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number of datepart units to add to the provided date.
 Number must be an integer.
 Negative integers move the date into the past, positive into the future.

### Argument: `date`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A datetime object in the range of 100AD-9999AD.
 NOTE: When passing a datetime object as a string, enclose it in quotation marks. Otherwise, it is interpreted as a numeric representation of a datetime object.

## Limitations and Other Info

- **Related Functions**: `dateConvert`, `datePart`, `createTimeSpan`, `createDate`, `dateDiff`
- **Coldfusion Support**: Notes: Member function is available in CF11+.
- **Lucee Support**: Notes: Member function is available in Lucee4.5+
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

