# Tag Name: `cfdocument`

## Description
Creates PDF or FlashPaper output from a text block containing CFML and HTML.

## Syntax
```cfml
<cfdocument format="PDF">html</cfdocument>
```

## Attributes / Variants

### Attribute: `format`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specifies the report format.

### Attribute: `filename`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the fully qualified path name of a file to
 contain the PDF or FlashPaper output. If you omit the
 filename attribute, ColdFusion MX streams output to
 the browser.

### Attribute: `localurl`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specifies whether to retrieve image files directly 
 from the local drive:
 * yes: ColdFusion retrieves image files directly from 
 the local drive rather than by using HTTP, HTTPS, or proxy.
 * no: ColdFusion uses HTTP, HTTPS, or proxy to retrieve 
 image files even if the files are stored locally.

### Attribute: `overwrite`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specifies whether ColdFusion MX overwrites an
 existing file. Used in conjunction with filename.
 Default is: false

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the name of an existing variable into which
 the tag stores the PDF or FlashPaper output.

### Attribute: `pagetype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `A4`
- **Description**: Specifies the page size into which ColdFusion
 generates the report.
 - legal: 8.5 inches x 14 inches
 - letter: 8.5 inches x 11 inches
 - A4: 8.27 inches x 11.69 inches
 - A5: 5.81 inches x 8.25 inches
 - B5: 9.81 inches x 13.88 inches
 - Custom: Custom height and width.
 If you specify custom, you must also specify the pageheight
 and pagewidth attributes, can optionally specify margin
 attributes, and can optionally specify whether the units
 are inches or centimeters.

### Attribute: `pageheight`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the page height in inches (default) or
 centimeters. This attribute is only valid if
 pagetype=custom. To specify page height in
 centimeters, include the unit=cm attribute.

### Attribute: `pagewidth`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the page width in inches (default) or
 centimeters. This attribute is only valid if
 pagetype=custom. To specify page width in
 centimeters, include the unit=cm attribute.

### Attribute: `orientation`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `portrait`
- **Description**: Specifies the page orientation. Specify either of the
 following:
 - portrait (default)
 - landscape

### Attribute: `margintop`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the top margin in inches (default) or
 centimeters. To specify top margin in centimeters,
 include the unit=cm attribute.

### Attribute: `marginbottom`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the bottom margin in inches (default) or
 centimeters. To specify bottom margin in
 centimeters, include the unit=cm attribute.

### Attribute: `marginleft`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the left margin in inches (default) or
 centimeters. To specify left margin in centimeters,
 include the unit=cm attribute.

### Attribute: `marginright`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the right margin in inches (default) or
 centimeters. To specify right margin in centimeters,
 include the unit=cm attribute.

### Attribute: `unit`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `in`
- **Description**: Specifies the default unit (inches or centimeters) for
 pageheight, pagewidth, and margin attributes.

### Attribute: `encryption`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `none`
- **Description**: Specifies whether the output is encrypted (format="PDF" only).
 Default is: none

### Attribute: `ownerpassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies an owner password (format="PDF" only).

### Attribute: `userpassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies a user password (format="PDF" only).

### Attribute: `permissions`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies one or more permissions (format="PDF" only).
 Separate multiple permissions with a comma.

### Attribute: `fontembed`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Specifies whether ColdFusion embeds fonts in the output.
 Specify one of the following:
 - true: Embed fonts
 - false: Do not embed fonts.
 Selective: Embed all fonts except Java fonts and core fonts.

### Attribute: `backgroundvisible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specifies whether the background prints when the
 user prints the document:
 - yes: include the background when printing.
 - no: do not include the background when printing.

### Attribute: `scale`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies a scale factor as a percentage. Use this
 option to reduce the size of the HTML output so that
 it fits on that paper. Specify a number less than 100.

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

### Attribute: `bookmark`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specifies whether bookmarks are created in the document:
 * yes: creates bookmarks.
 * no: does not create bookmarks.

### Attribute: `mimetype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `text/html`
- **Description**: MIME type of the source document

### Attribute: `proxypassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Password required by the proxy server.

### Attribute: `proxyuser`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: User name to provide to the proxy server.

### Attribute: `saveasname`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (format="PDF" only) The filename that appears in the SaveAs dialog when a user saves a PDF file written to the browser.

### Attribute: `src`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: URL or the relative path to the web root. You cannot specify both the src and srcfile attributes. 
The file must be in a browser-writable format such as, HTML, HTM, BMP, PNG, and so on.

### Attribute: `srcfile`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Absolute path of a file that is on the server. You cannot specify both the src and srcfile attributes. 
The file must be in a browser-writable format such as, HTML, HTM, BMP, PNG, and so on.

### Attribute: `useragent`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text to put in the HTTP User-Agent request header field. Used to identify the request client software.

### Attribute: `proxyhost`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: IP address or server name for proxy host.

### Attribute: `proxyport`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: port of the proxy host.

### Attribute: `tagged`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Determines if PDF are created by using special tags also known as Tagged PDF

### Attribute: `pdfa`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Creates a PDF of type PDF/A-1 (ISO 19005-1:2005)

### Attribute: `formFields`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Specifies whether form fields are exported as widgets or only their fixed print representation is exported.

### Attribute: `formsType`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `FDF`
- **Description**: Specifies the submitted format of a PDF form.

### Attribute: `permissionsPassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: This is the password that allows the user to access some permissions restricted if some permissions need to be restricted. The permissions are defined in "permissions" attribute

### Attribute: `openPassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If the source document is protected specify using this attribute.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

