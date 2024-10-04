# Function Name: `CreateDateTime`

## Description
Creates a date-time object.

## Return Type
`date`

## Syntax
```cfml
createDateTime(year, month, day, hour, minute, second)
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
- **Default Value**: `1`
- **Description**: Numeric month of the year (1-12).

### Argument: `day`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: `1`
- **Description**: Day of the month.

### Argument: `hour`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: `0`
- **Description**: Hour of the day in 24-hour notation (0-23).

### Argument: `minute`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: `0`
- **Description**: Minute within the hour.

### Argument: `second`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: `0`
- **Description**: Second within the minute.

### Argument: `millisecond`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: CF2021+ or Lucee4.5+ Only

### Argument: `timezone`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4.5+

## Limitations and Other Info

- **Related Functions**: `createDate`, `createTime`, `createODBCDateTime`
- **Coldfusion Support**: Minimum version: `6`. Notes: Since CF2016+ month, day, hour, minute are second are optional
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

