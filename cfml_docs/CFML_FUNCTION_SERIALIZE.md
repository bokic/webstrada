# Function Name: `Serialize`

## Description
Serializes the object to a specified type

## Return Type
`string`

## Syntax
```cfml
serialize( object, type, useCustomSerializer )
```

## Arguments

### Argument: `object`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An object to be serialized.

### Argument: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A type to which the object will be serialized. ColdFusion, by default supports XML and JSON formats. You can also implement support for other types in the CustomSerializer CFC.

### Argument: `useCustomSerializer`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Whether to use the custom serializer or not. The custom serializer will be always used for XML deserialization.
If false, the XML/JSON deserialization will be done using the default ColdFusion behavior.
If any other type is passed with useCustomSerializer as false, then TypeNotSupportedException will be thrown.

## Limitations and Other Info

- **Related Functions**: `deserialize`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:

