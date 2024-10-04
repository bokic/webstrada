# Function Name: `DeserializeXML`

## Description
Deserializes a string in XML format to a ColdFusion object.

## Return Type
`any`

## Syntax
```cfml
deserializeXML(string [,useCustomSerializer]);
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string that needs to be deserialized.

### Argument: `useCustomSerializer`
- **Type**: `boolean`
- **Required**: Required
- **Default Value**: `True`
- **Description**: This identifies whether or not to use the custom serializer. The default value is true. The custom serializer will be always used for XML deserialization. If false, the XML/JSON deserialization will be done using the default ColdFusion behavior. If any other type is passed with `useCustomSerializer` as false, then `TypeNotSupportedException` will be thrown.

## Limitations and Other Info

- **Related Functions**: `xmlParse`, `serializeXML`, `xmlNew`, `isXMLDoc`, `encodeForXML`
- **Coldfusion Support**: Minimum version: `11`.

