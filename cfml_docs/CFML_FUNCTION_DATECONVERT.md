# Function Name: `DateConvert`

## Description
Converts local time to Coordinated Universal Time (UTC), or UTC to local time. The function uses the daylight savings settings in the executing computer to compute daylight savings time, if required.

## Return Type
`date`

## Syntax
```cfml
dateConvert(conversionType, date)
```

## Arguments

### Argument: `conversionType`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: `local2Utc` : Converts local time to UTC time.
`utc2Local` : Converts UTC time to local time.

### Argument: `date`
- **Type**: `date`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

## Limitations and Other Info

- **Related Functions**: `getTimeZoneInfo`, `createDateTime`
- **Coldfusion Support**: Minimum version: `4`. Notes: Member function is available in CF11+.
- **Lucee Support**: Notes: Member function is not available.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

