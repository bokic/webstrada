# Tag Name: `cfmailpart`

## Description
Specifies one part of a multipart e-mail message. Can only be
 used in the cfmail tag. You can use more than one cfmailpart
 tag within a cfmail tag.

## Syntax
```cfml
<cfmailpart type="text/plain">
```

## Attributes / Variants

### Attribute: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The MIME media type of the part. Can be a valid MIME
 media type

### Attribute: `wraptext`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the maximum line length, in characters of the
 mail text. If a line has more than the specified number of
 characters, replaces the last white space character, such
 as a tab or space, preceding the specified position with a
 line break. If there are no white space characters,
 inserts a line break at the specified position. A common
 value for this attribute is 72.

### Attribute: `charset`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The character encoding in which the part text is encoded.

 For more information on character encodings, see:
 www.w3.org/International/O-charset.html.

## Limitations

- **Must be nested inside**: `cfmail`
- **Must not be nested inside**: *None*

