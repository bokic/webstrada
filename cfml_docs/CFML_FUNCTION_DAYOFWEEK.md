# Function Name: `DayOfWeek`

## Description
Determines the day of the week from a date. Returns the ordinal for the day of the week, as an integer in the range 1 (Sunday) to 7 (Saturday).

## Return Type
`numeric`

## Syntax
```cfml
dayOfWeek(date)
```

## Arguments

### Argument: `date`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Date or datetime object (100AD-9999AD).
When passing a datetime object as a string, enclose it in quotation marks. Otherwise, it is interpreted as a numeric representation of a datetime object.

### Argument: `calendar`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `gregorian`
- **Description**: CF2016u8+ Indicates whether the week starts on Sunday (gregorian) or Monday (iso)

## Limitations and Other Info

- **Related Functions**: `day`, `dayOfWeekAsString`, `dayOfYear`, `daysInMonth`, `daysInYear`, `firstDayOfMonth`
- **Coldfusion Support**: Notes: Member function is available in CF11+.
- **Lucee Support**: Notes: Member function is available in Lucee4.5+. Lucee accepts an additional argument for `timezone`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

