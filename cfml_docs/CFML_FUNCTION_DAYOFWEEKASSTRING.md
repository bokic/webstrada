# Function Name: `DayOfWeekAsString`

## Description
 Determines the day of the week as a string from 1-7

## Return Type
`string`

## Syntax
```cfml
dayOfWeekAsString(dayOfWeek [, locale])
```

## Arguments

### Argument: `dayOfWeek`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Only values from 1 to 7 are valid.
Week starting with 1 for Sunday and ends with 7 for Saturday:
	- 1 = Sunday 
	- 2 = Monday 
	- 3 = Tuesday 
	- 4 = Wednesday 
	- 5 = Thursday 
	- 6 = Friday 
	- 7 = Saturday

### Argument: `locale`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `Default JVM Locale`
- **Description**: Locale to use instead of the default JVM locale.

## Limitations and Other Info

- **Related Functions**: `dayOfWeekShortAsString`
- **Coldfusion Support**: Notes: CF2018: Renamed parameter `day_of_week` to `dayofweek`. 
 CF8: Added `locale` parameter. 
 CFMX7: Changed behavior. The returned string is now in the language of the current locale.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

