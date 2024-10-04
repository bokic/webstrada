# Function Name: `DeserializeJSON`

## Description
 Converts a JSON (JavaScript Object Notation) string data representation into CFML data, such as a CFML structure or array.

## Return Type
`any`

## Syntax
```cfml
deserializeJSON(json [, strictMapping, useCustomSerializer])
```

## Arguments

### Argument: `json`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string that contains a valid JSON construct or variable that represents one.

### Argument: `strictMapping`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: A Boolean value that specifies whether to convert the JSON strictly. If true, everything becomes structures.

### Argument: `useCustomSerializer`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: CF11+ Use custom serializer if defined. See: https://helpx.adobe.com/coldfusion/developing-applications/changes-in-coldfusion/restful-web-services-in-coldfusion.html

## Limitations and Other Info

- **Related Functions**: `serializeJSON`
- **Coldfusion Support**: Minimum version: `8`.
- **Lucee Support**: Notes: Lucee has only support for the first two parameters
- **Openbd Support**: Notes: In OpenBD it's possible to pass in a file path as third parameter instead of `useCustomSerializer` which will be used in place of the JSON passed with the first parameter
You can pass the parameters as a structure ("ArgumentCollection") as well
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: `JSONDeserialize` in BoxLang.  Works with `deserializeJSON` with the `bx-compat-cfml`

