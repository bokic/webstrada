# Function Name: `SetProfileString`

## Description
Sets the value of a profile entry in an initialization file.

## Return Type
`string`

## Syntax
```cfml
setProfileString(inipath, section, entry, value)
```

## Arguments

### Argument: `inipath`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Absolute path of initialization file

### Argument: `section`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Section of the initialization file in which the entry is
 to be set

### Argument: `entry`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the entry to set

### Argument: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Value to which to set the entry

## Limitations and Other Info

- **Related Functions**: `getProfileString`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-ini` module.

