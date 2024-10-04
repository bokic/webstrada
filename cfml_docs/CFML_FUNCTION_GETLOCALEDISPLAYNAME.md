# Function Name: `GetLocaleDisplayName`

## Description
Gets a locale value and displays the name in a manner that is appropriate to a specific locale. By default, gets the current locale in the current locale's language.

## Return Type
`string`

## Syntax
```cfml
getLocaleDisplayName([locale, inLocale])
```

## Arguments

### Argument: `locale`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The locale whose name you want. The default is the current ColdFusion locale, or if no ColdFusion locale has been set, the JVM locale.

### Argument: `inLocale`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The locale in which to return the name. The default is the current ColdFusion locale, or if no ColdFusion locale has been set, the JVM locale.

## Limitations and Other Info

- **Related Functions**: `getLocale`, `getLocaleInfo`, `getLocaleCountry`, `getLocaleLanguage`
- **Coldfusion Support**: Minimum version: `7`.
- **Lucee Support**: Notes: Lucee5+ Deprecated in favor of getLocaleInfo()
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

