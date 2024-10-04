# Function Name: `Week`

## Description
 From a date/time object, determines the week number within
 the year. An integer in the range 1-53; the ordinal of the
 week, within the year.

## Return Type
`numeric`

## Syntax
```cfml
week(date)
```

## Arguments

### Argument: `date`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A date/time object in the range 100 AD-9999 AD.

### Argument: `calendar`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `gregorian`
- **Description**: CF2016u8+ Indicates whether the week starts on Sunday (gregorian) or Monday (iso)

## Limitations and Other Info

- **Related Functions**: `hour`, `minute`, `second`, `month`, `quarter`, `year`
- **Coldfusion Support**: Notes: Member function is available in CF11+.
- **Lucee Support**: Notes: Member function is available in Lucee4.5+.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

