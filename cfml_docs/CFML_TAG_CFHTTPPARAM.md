# Tag Name: `cfhttpparam`

## Description
Allowed inside cfhttp tag bodies only. Required for cfhttp POST
 operations. Optional for all others. Specifies parameters to
 build an HTTP request.

## Syntax
```cfml
<cfhttpparam type="header">
```

## Attributes / Variants

### Attribute: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The type of data to send

 `header`: Specifies an HTTP header. Does not URL encode the value

`body`: Specifies that the `value` is the body of the HTTP request.

`xml`: Identifies the request as having a content-type of
 `text/xml` and specifies that the `value` attribute contains the body of the HTTP request.

`cgi`: Same as `header` but URL encodes the `value` by default.

`file`: Tells CFML to send the contents of the specified file.

`url`: Specifies a URL query string name-value pair to append to the cfhttp url attribute. URL encodes the value.

`formfield`: Specifies a form field to send. URL encodes the value by default.

`cookie`: Specifies a cookie to send as an HTTP header. URL encodes the value.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Variable name for data that is passed. Ignored for `body` and `xml` type. For `file` type, specifies the filename.

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Value of the data that is sent. Ignored for `file` type. The value must contain string data or data that CFML can convert to a string for all type attributes except Body. Body types can have string or binary values.

### Attribute: `file`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies to `file` type; ignored for all other types. The absolute path to the file that is sent with the request.

### Attribute: `encoded`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies to `formfield` and `cgi` types; ignored for all other
 types. Specifies whether to URLEncode the form field or
 header.

### Attribute: `mimetype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies to `file` type; invalid for all other types.
 Specifies the MIME media type of the file contents.
 The content type can include an identifier for the
 character encoding of the file; for example, text/html;
 charset=ISO-8859-1 indicates that the file is HTML text in
 the ISO Latin-1 character encoding.

## Limitations

- **Must be nested inside**: `cfhttp`
- **Must not be nested inside**: *None*

