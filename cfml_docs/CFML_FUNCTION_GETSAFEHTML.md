# Function Name: `GetSafeHTML`

## Description
Sanitizes HTML using antisamy policy rules. 

## Return Type
`any`

## Syntax
```cfml
getSafeHTML(inputString [, PolicyFile, throwOnError])
```

## Arguments

### Argument: `inputString`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String to be sanitized

### Argument: `PolicyFile`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: File path for custom antisamy policy file. Can be defined in the application scope or if not defined will use ColdFusion server default

### Argument: `throwOnError`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If true will throw error else empty string will be returned

## Limitations and Other Info

- **Related Functions**: `isSafeHTML`
- **Coldfusion Support**: Minimum version: `11`.
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-esapi` module.

