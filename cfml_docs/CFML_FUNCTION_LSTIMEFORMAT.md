# Function Name: `LSTimeFormat`

## Description
Formats the time part of a date/time string into a string in a locale-specific format.
 [mask - quicky]
 h,hh,H,HH: Hours; m,mm: Minutes; s,ss: Seconds;
 l: Milliseconds; t: A or P; tt: AM or PM
 "short" = h:mm tt; "medium" = h:mm:ss tt

## Return Type
`string`

## Syntax
```cfml
lsTimeFormat(time [, mask, locale])
```

## Arguments

### Argument: `time`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `mask`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `short`
- **Description**: 

### Argument: `locale`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Geographic/language locale value, where the format is a combination of an ISO 639-1 code and an optional ISO 3166-1 code separated by a dash or an underscore.

## Limitations and Other Info

- **Related Functions**: `lsDateFormat`, `timeFormat`
- **Coldfusion Support**: Notes: Member function is available in CF2016+.
- **Lucee Support**: Notes: Member function is not available.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

