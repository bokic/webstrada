# Function Name: `GetProfileString`

## Description
Gets an initialization file entry. An initialization file assigns values to configuration variables, also known as entries, that are set when the system
 boots, the operating system comes up, or an application starts. Returns the entry - if no value, returns an empty string.

## Return Type
`string`

## Syntax
```cfml
getProfileString(inipath, section, entry)
```

## Arguments

### Argument: `inipath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `section`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `entry`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

## Limitations and Other Info

- **Related Functions**: `setProfileString`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-ini` module.

