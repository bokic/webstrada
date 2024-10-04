# Function Name: `isSafeHTML`

## Description
Checks a HTML string against antisamy policy file to determine if it may be vulnerable to XSS / Cross Site Scripting.

## Return Type
`boolean`

## Syntax
```cfml
isSafeHTML(inputString [, PolicyFile])
```

## Arguments

### Argument: `inputString`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: String to be validated

### Argument: `PolicyFile`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: File path for custom AntiSamy policy file. Can be defined in the application scope or if not defined will use ColdFusion server default

## Limitations and Other Info

- **Related Functions**: `getSafeHTML`
- **Coldfusion Support**: Minimum version: `11`.
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Requires the `bx-esapi` module.

