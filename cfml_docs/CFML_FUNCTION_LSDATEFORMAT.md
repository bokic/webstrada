# Function Name: `LSDateFormat`

## Description
Formats the date part of a date/time value in a locale-specific format.
 [mask - quicky]
 d,dd,ddd,dddd: Day of month / week
 m,mm,mmm,mmmm: Month
 y,yy,yyyy: Year
 gg: Period/era string
 short / medium / long / full

## Return Type
`string`

## Syntax
```cfml
lsDateFormat(date [, mask, locale])
```

## Arguments

### Argument: `date`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The datetime object

### Argument: `mask`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `medium`
- **Description**: A keyword or custom combination of components

### Argument: `locale`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF8+ Locale to use instead of the locale of the page when processing the function

## Limitations and Other Info

- **Related Functions**: `lsTimeFormat`, `dateFormat`
- **Coldfusion Support**: Notes: Member function is available in CF11+.
- **Lucee Support**: Notes: Member function is available in Lucee4.5+.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

