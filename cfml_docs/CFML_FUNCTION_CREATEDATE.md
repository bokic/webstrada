# Function Name: `CreateDate`

## Description
Creates a date/time object

## Return Type
`date`

## Syntax
```cfml
createDate(year, month, day)
```

## Arguments

### Argument: `year`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Integer in the range 0-9999. When in the range 0-29, year is converted to 2000-2029. When in the range 30-99, year is converted to 1930-1999. You cannot specify dates before AD 100.

### Argument: `month`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Numeric month of the year (1-12)

### Argument: `day`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Day of the month

## Limitations and Other Info

- **Related Functions**: `createDateTime`, `createODBCDate`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

