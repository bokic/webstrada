# Function Name: `DateFormat`

## Description
Formats a date value using U.S. date formats. When formatting both date and time, use dateTimeFormat. For international date support, use lsDateFormat.

## Return Type
`string`

## Syntax
```cfml
dateFormat(date [, mask])
```

## Arguments

### Argument: `date`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The datetime object (100AD-9999AD)

### Argument: `mask`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `dd-mmm-yy`
- **Description**: Each keyword below will be replaced in the string by its respective datepart:
	-d: Day of the month as digits; no leading zero for single-digit days.
	-dd: Day of the month as digits; leading zero for single-digit days.
	-ddd: Day of the week as a three-letter abbreviation.
	-dddd: Day of the week as its full name.
	-e: Day in a week (CF2016u3+).
	-f: Day of a week in a month (CF2016u3+).
	-D: Day in year. (pre-CF2016u3)
	-m: Month as digits; no leading zero for single-digit months.
	-mm: Month as digits; leading zero for single-digit months.
	-mmm: Month as a three-letter abbreviation.
	-mmmm: Month as its full name.
	-M: Month in year. (pre-CF2016u3)
	-k: Hour in a day (CF2016u3+).
	-W: Week in a month (CF2016u3+).
	-w: Week in a year (CF2016u3+).
	-yy: Year as last two digits; leading zero for years less than 10.
	-yyyy,YYYY: Year represented by four digits.
	-gg: Period/era string.
	-z: Time zone in unstandardized abbreviated format, for example, `EST` (CF2016u3+).
	-Z: Time zone in hours of offset (RFC 822 TimeZone), for example, `+0530` (CF2016u3+).
	-X: Time zone in hours of offset in ISO 8601 format. (CF2016u3+).
	-X: `+05`
	 -XX: `+0530`
	 -XXX: `+5:30`
 The following keywords are shorthand for specific full formats and cannot be combined with other masks:
	-short: equivalent to `m/d/y`
	-medium: equivalent to `mmm d, yyyy`
	-long: equivalent to `mmmm d, yyyy`
	-full: equivalent to `dddd, mmmm d, yyyy`

## Limitations and Other Info

- **Related Functions**: `timeFormat`, `dateTimeFormat`, `lsDateFormat`
- **Coldfusion Support**: Notes: CF6+ Added support for the `short`, `medium`, `long` and `full` mask formats. Member function is available in CF11+.
- **Lucee Support**: Notes: Member function is available in Lucee4.5+.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

