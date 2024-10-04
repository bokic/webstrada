# Function Name: `LSCurrencyFormat`

## Description
Formats a number in a locale-specific currency format. For countries that use the euro, the result depends on the JVM.
 [type - quicky]
 local: the currency format and currency symbol used locally.
 - With JDK 1.3, the default for Euro Zone: local currency.
 - With JDK 1.4, the default for Euro Zone: euro.
 international: the international standard currency format
 none: the local currency format; no currency symbol

## Return Type
`string`

## Syntax
```cfml
lsCurrencyFormat(number [, type, locale])
```

## Arguments

### Argument: `number`
- **Type**: `numeric`
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

- **Related Functions**: `dollarFormat`, `LSEuroCurrencyFormat`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

