# Function Name: `LSNumberFormat`

## Description
Formats a number in a locale-specific format.
[mask - quicky]
 _,9 Digit placeholder; . decimal point; 0 Pads with zeros;
 ( ) less than zero, puts parentheses around the mask
+ plus sign before positive number minus before negative
- a space before positive minus sign before negative
, Separates every third decimal place with a comma.
L,C Left-justifies or center-justifies number
$ dollar sign before formatted number.
^ Separates left and right formatting.

## Return Type
`string`

## Syntax
```cfml
lsNumberFormat(number [, mask, locale])
```

## Arguments

### Argument: `number`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `mask`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `_`
- **Description**: 

### Argument: `locale`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Geographic/language locale value, where the format is a combination of an ISO 639-1 code and an optional ISO 3166-1 code separated by a dash or an underscore.

## Limitations and Other Info

- **Related Functions**: `numberFormat`, `decimalFormat`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

