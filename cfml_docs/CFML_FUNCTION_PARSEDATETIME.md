# Function Name: `ParseDateTime`

## Description
 Parses a date/time string according to the English (U.S.)
 locale conventions. (To format a date/time string for other
 locales, use the LSParseDateTime function.)

## Return Type
`date`

## Syntax
```cfml
parseDateTime(dt_string [, pop_conversion])
```

## Arguments

### Argument: `dt_string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `pop_conversion`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `standard`
- **Description**: - standard: function does no conversion.
 - pop: specifies that the date/time string is in POP format, which includes the local time of the sender and a time-zone offset from UTC. ColdFusion applies the offset and returns
 a value with the UTC time.
- Apart from these two values, this parameter allows you to directly specify the format in which to parse the given string.

## Limitations and Other Info

- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

