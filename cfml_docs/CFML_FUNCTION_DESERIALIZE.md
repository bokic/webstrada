# Function Name: `Deserialize`

## Description
Deserializes a string.

## Return Type
`string`

## Syntax
```cfml
deserialize(string, type, useCustomSerializer);
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string that needs to be deserialized.

### Argument: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The type of the data to be deserialized. ColdFusion by default supports XML and JSON formats. You can also implement support for other types in the CustomSerializer CFC.

### Argument: `useCustomSerializer`
- **Type**: `boolean`
- **Required**: Required
- **Default Value**: `true`
- **Description**: Whether to use the custom serializer or not. The custom serializer will always be used for deserialization.
If false, the XML/JSON deserialization will be done using the default ColdFusion behavior.
If any other type is passed with `useCustomSerializer` as false, then `TypeNotSupportedException` will be thrown.

## Limitations and Other Info

- **Related Functions**: `serialize`
- **Coldfusion Support**: Minimum version: `11`.

