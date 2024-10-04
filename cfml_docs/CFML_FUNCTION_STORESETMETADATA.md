# Function Name: `StoreSetMetadata`

## Description
Sets the metadata on bucket or object.

## Return Type
`void`

## Syntax
```cfml
storeSetMetadata(url,Struct);
```

## Arguments

### Argument: `url`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Amazon S3 URLs (bucket or object).

### Argument: `region`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Represents the metadata. See Standard keys for a list of standard keys in metadata. You can also have custom metadata apart from the standard ones.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `9.0.1`.
- **Lucee Support**:

