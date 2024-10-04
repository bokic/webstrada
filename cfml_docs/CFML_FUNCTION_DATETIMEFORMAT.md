# Function Name: `DateTimeFormat`

## Description
Formats a datetime value using U.S. date and time formats. For international date support, use lsDateTimeFormat.

## Return Type
`string`

## Syntax
```cfml
dateTimeFormat(date [, mask [, timezone]])
```

## Arguments

### Argument: `date`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The datetime object (100AD-9999AD).
 NOTE: This parameter is named `datetime` in Lucee.

### Argument: `mask`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `dd-mmm-yyyy HH:nn:ss`
- **Description**: The mask used to format the datetime output.
	- d : Day of the month as digits; no leading zero for single-digit days.
	- dd : Day of the month as digits; leading zero for single-digit days.
	- EEE : Day of the week as a three-letter abbreviation.
	- EEEE : Day of the week as its full name.
	- m : Month as digits; no leading zero for single-digit months.
	- mm : Month as digits; leading zero for single-digit months.
	- mmm : Month as a three-letter abbreviation. (Dec)
	- mmmm : Month as its full name.
	- M : Month in year. (pre-CF2016u3)
	- D : Day in year. (pre-CF2016u3)
	- yy : Year as last two digits; leading zero for years less than 10.
	- yyyy : Year represented by four digits.
	- YYYY : Week year represented by four digits. (pre-CF2016u3)
	- Y : Week year. (pre-CF2016u3)
	- YY : Week Year as last two digits; leading zero for years less than 10. (pre-CF2016u3)
	- G : Period/era string. (e.g. BC/AD)
	- h : hours; no leading zero for single-digit hours. (12-hour clock)
	- hh : hours; leading zero for single-digit hours. (12-hour clock)
	- H : hours; no leading zero for single-digit hours. (24-hour clock)
	- HH : hours; leading zero for single-digit hours. (24-hour clock)
	- n : minutes; no leading zero for single-digit minutes.
	- nn : minutes; leading zero for single-digit minutes.
	- s : seconds; no leading zero for single-digit seconds.
	- ss : seconds; leading zero for single-digit seconds.
	- l or L : milliseconds, with no leading zeros.
	- t : one-character time marker string, such as A or P.
	- tt : multiple-character time marker string, such as AM or PM.
	- w : Week of the year as digit. (JDK7+)
	- ww : Week of the year as digits; leading zero for single-digit week. (JDK7+)
	- W : Week of the month as digit. (JDK7+)
	- WW : Week of the month as digits; leading zero for single-digit week. (JDK7+)

The following masks tell how to format the full date and time and cannot be combined with other masks:
	- `short` : equivalent to `"m/d/y h:nn tt"`
	- `medium` : equivalent to `"mmm d, yyyy h:nn:ss tt"`
	- `long` : `medium` with full month name rather than abbreviation, followed by three-letter time zone; as in, `"mmmm d, yyyy h:mm:ss tt EST"`
	- `full` : equivalent to `"EEEE, mmmm d, yyyy h:mm:ss tt EST"`
	- `iso` CF2016+ Lucee5.3.8+ Formats the date time in ISO8601 format
	- `iso8601` Lucee4.5+ Formats the date time in ISO8601 format

The function also follows Java date time mask, except for mask "a". For more information, refer to Date and Time Patterns topic in `SimpleDateFormat` Java API page.

JDK7 and JDK8 introduces the masks "w", "ww", "W", and "WW".

### Argument: `timezone`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `System time-zone`
- **Description**: The timezone to use. Can be the 3 letter code ("EST","UTC") or the full name full {"America/New_York")

## Limitations and Other Info

- **Related Functions**: `dateFormat`, `timeFormat`, `datetime`
- **Coldfusion Support**: Minimum version: `10`. Notes: Member function is available in CF11+.
- **Lucee Support**: Minimum version: `4.5`. Notes: The timezone argument does not appear to convert the date from the system timezone to the specified timezone as it does in ACF. Member function is available in Lucee5+.
- **Railo Support**: Minimum version: `4.0.1`. Notes: No documentation exists for this function on Railo, however LSDateTimeFormat is documented. Follows Java date time mask. For details, see the section Date and Time Patterns at the following URL: http://docs.oracle.com/javase/1.4.2/docs/api/java/text/SimpleDateFormat.html 
- **Openbd Support**: Notes: Some of the mask tokens differ from Adobe ColdFusion
- **Boxlang Support**: Minimum version: `1.0.0`.

