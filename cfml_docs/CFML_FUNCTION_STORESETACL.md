# Function Name: `StoreSetACL`

## Description
 Sets the ACL for object or bucket.

## Return Type
`void`

## Syntax
```cfml
storeSetACL(url, ACLObject);
```

## Arguments

### Argument: `url`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Amazon S3 URLs (content or object).

### Argument: `ACLObject`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: An array of struct where each struct represents a permission or grant as discussed in ACLObject.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `9.0.1`.
- **Lucee Support**:
- **Railo Support**:

