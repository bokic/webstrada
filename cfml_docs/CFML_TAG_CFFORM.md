# Tag Name: `cfform`

## Description
Builds a form with CFML custom control tags; these provide
 more functionality than standard HTML form input elements.

## Syntax
```cfml
<cfform>
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: In HTML format, if you omit this attribute and specify
 an id attribute, ColdFusion does not include a name
 attribute in the HTML sent to the browser; this
 behavior lets you use the cfform tag to create
 XHTML-compliant forms. If you omit the name
 attribute and the id attribute, ColdFusion generates
 a name of the form CFForm_n where n is a number
 that assigned serially to the forms on a page.

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of CFML page to execute when the form is
 submitted for processing.

### Attribute: `method`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `post`
- **Description**: The method the browser uses to send the form data
 to the server:
 - post: Send the data using the HTTP post method,
 This method sends the data in a separate message
 to the server.
 - get: Send the data using the HTTP get method,
 which puts the form field contents in the URL
 query string.

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `html`
- **Description**: - `HTML`: Generate an HTML form and send it to the client. 
 - `Flash`: DEPRECATED in CF11+ Generate a Flash form and send it to the client. All controls are in Flash format.
 - `XML`: DEPRECATED in CF11+ Generate XForms-compliant XML and save
 the results in a variable specified by the name
 attribute. By default, ColdFusion also applies an XSL skin and displays the result.

### Attribute: `skin`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: DEPRECATED in CF11+
 `Flash`: Use a Macromedia halo color to stylize the output.
 `XML`: Specifies whether to apply an XSL skin and
 display the resulting HTML to the client. Can be any
 of the following:
 - ColdFusion MX skin name: Apply the specified skin.
 - XSL file name: Apply the skin located in the specified path.
 - "none": Do not apply an XSL skin. You must use XForms XML then.
 - (omitted) or "default": Use the ColdFusion MX default skin.

### Attribute: `preservedata`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: When the cfform action attribute posts back to the same
 page as the form, this determines whether to override the
 control values with the submitted values.
 - false: values specified in the control tag attributes are used
 - true: corresponding submitted values are used

### Attribute: `onload`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript to execute when the form loads.

### Attribute: `onsubmit`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript or Actionscript function to execute to
 preprocess data before form is submitted. If any
 child tags specify onSubmit field validation, ColdFusion
 does the validation before executing this JavaScript.

### Attribute: `codebase`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `/CFIDE/classes/cf-j2re-win.cab`
- **Description**: URL of downloadable JRE plug-in (for Internet Explorer only).
 Default: /CFIDE/classes/cf-j2re-win.cab

### Attribute: `archive`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `/CFIDE/classes/cfapplets.jar`
- **Description**: URL of downloadable Java classes for CFML controls.
 Default: /CFIDE/classes/cfapplets.jar

### Attribute: `height`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `100%`
- **Description**: The height of the form. Use a number to specify
 pixels, In Flash, you can use a percentage value to
 specify a percentage of the available width. The
 displayed height might be less than the specified size.

### Attribute: `width`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `100%`
- **Description**: The width of the form. Use a number to specify
 pixels, In Flash, you can use a percentage value to
 specify a percentage of the available width.

### Attribute: `onerror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies only for onSubmit or onBlur validation; has
 no effect for onServer validation. An ActionScript
 expression or expressions to execute if the user
 submits a form with one or more validation errors.

### Attribute: `wmode`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `window`
- **Description**: Specifies how the Flash form appears relative to
 other displayable content that occupies the same
 space on an HTML page.
 - window: The Flash form is the topmost layer on the
 page and obscures anything that would share the
 space, such as drop-down dynamic HTML lists.
 - transparent: The Flash form honors the z-index of
 DHTML so you can float items above it. If the Flash
 form is above any item, transparent regions in the
 form show the content that is below it.
 - opaque: The Flash form honors the z-index of
 DHTML so you can float items above it. If the Flash
 form is above any item, it blocks any content that is
 below it.
 Default is: window.

### Attribute: `accessible`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specifies whether to include support screen readers
 in the Flash form. Screen reader support adds
 approximately 80KB to the SWF file sent to the
 client. Default is: false.

### Attribute: `preloader`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Specifies whether to display a progress bar when
 loading the Flash form. Default is: true.

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Integer number of seconds for which to keep the
 form data in the Flash cache on the server. A value of
 0 prevents the data from being cached.

### Attribute: `scriptsrc`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the URL, relative to the web root, of the
 directory that contains the cfform.js file with the
 client-side JavaScript used by this tag and its child
 tags. For XML format forms, this directory is also the
 default directory for XSLT skins.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Styles to apply to the form. In HTML or XML format,
 ColdFusion passes the style attribute to the browser
 or XML. In Flash format, must be a style specification
 in CSS format.

### Attribute: `onreset`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: JavaScript to execute when the user clicks a reset button.

### Attribute: `id`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: HTML id passed through to <FORM>.

### Attribute: `target`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Target window or frame passed through to <FORM>.

### Attribute: `passthrough`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: DEPRECATED in CF7+ REMOVED in CF11+ Passes arbitrary attribute-value pairs to the HTML code
 that is generated for the tag. You can use either of the
 following formats:
 
 passthrough="title=""myTitle"""
 passthrough='title="mytitle"'

### Attribute: `onsuccess`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Applies only to forms inside cfdiv, cflayout, cfpod, or cfwindow controls. The name of a JavaScript function that will run when an asynchronous form submission succeeds.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

