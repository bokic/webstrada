# Function Name: `DateDiff`

## Description
Determines the integer number of datepart units by which date1 is less than date2.

## Return Type
`numeric`

## Syntax
```cfml
dateDiff(datepart, date1, date2)
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
 w: Week (Weekday cf2018+)
 ww: Week
 h: Hour
 n: Minute
 s: Second


### Argument: `date1`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The smaller date to diff
Can be either a string or a date object whereas member function only accept the latter

### Argument: `date2`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The bigger date to diff
Can be either a string or a date object whereas member function only accept the latter

## Limitations and Other Info

- **Related Functions**: `dateadd`, `dateformat`
- **Coldfusion Support**: Notes: Member function is available in CF11+.
- **Lucee Support**: Notes: Member function is available in Lucee4.5+. The Lucee member function diffs dates in the opposite direction (+/-) than the Adobe CF member function. See the example below. This behaviour was changed in Lucee 6.0.0.130 to match ACF.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

