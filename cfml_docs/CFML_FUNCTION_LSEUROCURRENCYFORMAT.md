# Function Name: `LSEuroCurrencyFormat`

## Description
Formats a number in a locale-specific currency format.
 [type - quicky]
 local: the currency format used in the locale. (Default.)
 international: the international standard currency format
 none: the currency format used; no currency symbol

## Return Type
`string`

## Syntax
```cfml
lsEuroCurrencyFormat(currency [, type, locale])
```

## Arguments

### Argument: `currency`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `local`
- **Description**: 

### Argument: `locale`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Geographic/language locale value, where the format is a combination of an ISO 639-1 code and an optional ISO 3166-1 code separated by a dash or an underscore.

## Limitations and Other Info

- **Related Functions**: `LSCurrencyFormat`, `dollarFormat`
- **Coldfusion Support**: Minimum version: `4`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

