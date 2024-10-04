# Function Name: `DatePart`

## Description
Extracts a part from a datetime value as a numeric.

## Return Type
`numeric`

## Syntax
```cfml
datePart(datepart, date [,timezone])
```

## Arguments

### Argument: `datepart`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: yyyy: Year 
 q: Quarter 
 m: Month 
 y: Day of year 
 d: Day 
 w: Weekday 
 ww: Week 
 h: Hour 
 n: Minute 
 s: Second 
 l: Millisecond

### Argument: `date`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Datetime object (100AD-9999AD).

### Argument: `timezone`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `Timezone Specified in Lucee Administrator`
- **Description**: This is only available in Lucee.
  A datetime object is independent of a specific timezone; it is only an offset in milliseconds from `1970-1-1 00.00:00 UTC`.
 The timezone only comes into play when you need specific information like hours in a day, minutes in an hour or which day it is, as these calculations depend on the timezone.
 A timezone must be specified in order to translate the date object to something else. If you do not provide the timezone in the function call, it will default to the timezone specified in the Lucee Administrator (Settings/Regional), or the timezone specified for the current request using the function `setTimezone()`.

## Limitations and Other Info

- **Related Functions**: `year`, `quarter`, `month`, `dayOfYear`, `day`, `dayOfWeek`, `week`, `hour`, `minute`, `second`
- **Coldfusion Support**: Notes: Member function is available is CF11+.
- **Lucee Support**: Notes: Member function is available is Lucee4.5+. The syntax for this member function is `date.part(datepart)`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

