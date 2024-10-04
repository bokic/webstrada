# Function Name: `LSDateTimeFormat`

## Description
 Formats date and time values using locale-specific date and time formatting conventions.

## Return Type
`string`

## Syntax
```cfml
lsDateTimeFormat(date [, mask, locale, [timeZone]]);
```

## Arguments

### Argument: `date`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A date/time object, in the range 100 AD-9999 AD.

### Argument: `mask`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Mask that has to be used for formatting.

### Argument: `locale`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Locale to use instead of the locale of the page when processing the function.

### Argument: `timeZone`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The time-zone information. You can specify in either of the following formats. Abbreviation and Full Name.

## Limitations and Other Info

- **Related Functions**: `dateTimeFormat`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Railo Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

