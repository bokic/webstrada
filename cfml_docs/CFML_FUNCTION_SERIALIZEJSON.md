# Function Name: `SerializeJSON`

## Description
Converts a ColdFusion value into a JSON (JavaScript Object Notation) string.

## Return Type
`string`

## Syntax
```cfml
serializeJSON(data[, queryFormat[, useSecureJSONPrefix[, useCustomSerializer]]])
```

## Arguments

### Argument: `data`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A serializable ColdFusion data value

### Argument: `queryFormat`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: `row`
- **Description**: This specifies how to serialize ColdFusion queries. Prior to CF11+, this would only accept Boolean values. If it is a Boolean, the false value is equivalent to 'row' and true is 'column'.

### Argument: `useSecureJSONPrefix`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF11+ When Prefix Serialized JSON is enabled in the ColdFusion Administrator, then by default this function inserts the secure JSON prefix at the beginning of the JSON.

### Argument: `useCustomSerializer`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: CF11+ Use custom serializer if defined. See: https://helpx.adobe.com/coldfusion/developing-applications/changes-in-coldfusion/restful-web-services-in-coldfusion.html

## Limitations and Other Info

- **Related Functions**: `deserializeJSON`, `structSetMetadata`, `arraySetMetadata`
- **Coldfusion Support**: Minimum version: `8`. Notes: CF11+ Added `useSecureJSONPrefix` and `useCustomSerializer` arguments. Changed `queryFormat` (formerly known as `serializeQueryByColumns`) from a boolean type to also allowing strings, adding the 'struct' functionality in the process. The default for `queryFormat` can be set using the newly added Application.cfc setting `this.serialization.serializeQueryAs`. Also added Application.cfc settings `this.serialization.preserveCaseForStructKey` and `this.serialization.preserveCaseForQueryColumn`. CF2016+ Added member syntax.
- **Lucee Support**: Notes: Member syntax added for Lucee5.3.8+
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`. Notes: Transpiled to `JSONSerialize` in BoxLang.

