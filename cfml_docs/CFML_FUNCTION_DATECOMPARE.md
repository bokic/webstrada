# Function Name: `DateCompare`

## Description
Performs a full date/time comparison of two dates.
 `-1` if date1 is less than date2
 `0` if date1 is equal to date2
 `1` if date1 is greater than date2
 [DatePart] `yyyy`: Year; `m`: Month; `d`: Day; `h`: Hour; `n`: Minute; `s`: Second

## Return Type
`numeric`

## Syntax
```cfml
dateCompare(date1, date2 [, datePart])
```

## Arguments

### Argument: `date1`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A date to compare

### Argument: `date2`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Another date to compare

### Argument: `datePart`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `s`
- **Description**: 

## Limitations and Other Info

- **Related Functions**: `compare`, `compareNoCase`, `dateDiff`
- **Coldfusion Support**:
- **Lucee Support**: Notes: In Lucee "y" can be used instead of "yyyy"
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

