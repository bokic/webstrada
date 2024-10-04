# Function Name: `NumberFormat`

## Description
Creates a custom-formatted number value. For international
number formatting use LSNumberFormat.
The mask is made up of:
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
numberFormat(number [, mask])
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
- **Default Value**: *None*
- **Description**: _ : (underscore): Optional. Digit placeholder.
9 : Optional. Digit placeholder. (Shows decimal places more clearly than _.). Note that when ‘9’ or ‘_’  is used after the decimal place, 0 will be padded, if required. 
. : Location of a mandatory decimal point.
0 : Located to the left or right of a mandatory decimal point. Pads with zeros.
( ) : If number is less than zero, puts parentheses around the mask.
+ : Puts plus sign before positive number; minus sign before negative number.
- : Puts a space before positive number; minus sign before negative number.
, : Separates every third decimal place with a comma.
L,C : Left-justifies or center-justifies number within width of mask column. First character of mask must be L or C. The default value is right-justified.
$ : Puts a dollar sign before formatted number. First character of mask must be the dollar sign ($).
^ : Separates left and right formatting.

## Limitations and Other Info

- **Related Functions**: `lsNumberFormat`, `decimalFormat`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

