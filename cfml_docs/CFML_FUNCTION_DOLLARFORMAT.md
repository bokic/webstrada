# Function Name: `DollarFormat`

## Description
Formats a string in U.S. Dollar format. For other currencies, use `LSCurrencyFormat` or `LSEuroCurrencyFormat`. The function will return a number as a string, formatted with two decimal places, thousands separator and dollar sign. If the number is negative, the return value is enclosed in parentheses. If number is an empty string, the function returns zero.

## Return Type
`string`

## Syntax
```cfml
dollarFormat(number)
```

## Arguments

### Argument: `number`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The number to format.

## Limitations and Other Info

- **Related Functions**: `LSCurrencyFormat`, `LSEuroCurrencyFormat`, `numberFormat`
- **Coldfusion Support**: Notes: DollarFormat Rounding Bug CF-4199995 - Certain inputs do not cause dollarFormat to round up, for example 6.5850 results in $6.58, yet 6.585 results in $6.59. Consider using numberFormat as a workaround.
https://tracker.adobe.com/#/view/CF-4199995
- **Lucee Support**: Notes: This bug mentioned for ACF also existed in Lucee 4.5, but seems to be fixed in Lucee 5.
https://luceeserver.atlassian.net/browse/LDEV-574
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

