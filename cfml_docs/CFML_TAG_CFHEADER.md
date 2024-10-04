# Tag Name: `cfheader`

## Description
Generates custom HTTP response headers to return to the client.

## Syntax
```cfml
<cfheader>
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Header name
 Required if statusCode not specified

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: HTTP header value

### Attribute: `charset`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The character encoding in which to encode the header value.

 For more information on character encodings, see:
 www.w3.org/International/O-charset.html.

### Attribute: `statuscode`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: HTTP status code
 Required if name not specified

### Attribute: `statustext`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Explains status code
Removed in CF2025 as it's also removed in Tomcat.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

