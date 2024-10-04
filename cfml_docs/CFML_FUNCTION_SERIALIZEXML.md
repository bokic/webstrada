# Function Name: `SerializeXML`

## Description
Serializes the given object to XML.

## Return Type
`string`

## Syntax
```cfml
serializeXML( objToBeSerialized, useCustomSerializer )
```

## Arguments

### Argument: `objToBeSerialized`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An object to be serialized.

### Argument: `useCustomSerializer`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Whether to use the custom serializer. The default value is true. The custom serializer will be always used for XML deserialization. If false, the XML/JSON deserialization will be done using the default ColdFusion behavior. If any other type is passed with useCustomSerializer as false, then TypeNotSupportedException will be thrown.

## Limitations and Other Info

- **Related Functions**: `xmlParse`, `xmlNew`, `deserializeXML`, `isXMLDoc`, `encodeForXML`
- **Coldfusion Support**: Minimum version: `11`.

