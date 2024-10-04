# Tag Name: `cfdocumentsection`

## Description
Divides a PDF or FlashPaper document into sections.
 By using this tag in conjunction with a cfdocumentitem
 tag, each section can have unique headers, footers,
 and page numbers.

## Syntax
```cfml
<cfdocumentsection>
```

## Attributes / Variants

### Attribute: `margintop`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the top margin in inches (default) or
 centimeters. To specify the top margin in centimeters,
 include the unit="cm" attribute in the parent cfdocument
 tag.

### Attribute: `marginbottom`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the bottom margin in inches (default) or
 centimeters. To specify the bottom margin in
 centimeters, include the unit="cm" attribute in the
 parent cfdocument tag.

### Attribute: `marginleft`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the left margin in inches (default) or
 centimeters. To specify the left margin in centimeters,
 include the unit="cm" attribute in the parent cfdocument
 tag.

### Attribute: `marginright`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the right margin in inches (default) or
 centimeters. To specify the right margin in centimeters,
 include the unit="cm" attribute in the parent cfdocument
 tag.

### Attribute: `authpassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Password sent to the target URL for Basic Authentication. 
Combined with username to form a base64 encoded string that is passed in the Authenticate header. 
Does not provide support for Integrated Windows, NTLM, or Kerebos authentication.

### Attribute: `authuser`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: User name sent to the target URL for Basic Authentication. 
Combined with password to form a base64 encoded string that is passed in the Authenticate header. 
Does not provide support for Integrated Windows, NTLM, or Kerebos authentication.

### Attribute: `mimetype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `text/html`
- **Description**: MIME type of the source document

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Bookmark name for the section.

### Attribute: `src`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: URL or the relative path to the web root. You cannot specify both the src and srcfile attributes.

### Attribute: `srcfile`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Absolute path of a file that is on the server. 
You cannot specify both the src and srcfile attributes.

### Attribute: `useragent`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text to put in the HTTP User-Agent request header field. Used to identify the request client software.

## Limitations

- **Must be nested inside**: `cfdocument`
- **Must not be nested inside**: *None*

