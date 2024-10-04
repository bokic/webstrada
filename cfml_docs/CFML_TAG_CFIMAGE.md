# Tag Name: `cfimage`

## Description
Creates a ColdFusion image that can be manipulated by using image functions.
You can use the cfimage tag to perform common image manipulation operations as a shortcut to Image functions.
You can use the cfimage tag independently or in conjunction with image functions.

## Syntax
```cfml
<cfimage>
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `read`
- **Description**: The action to take.

### Attribute: `angle`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Angle in degrees to rotate the image.

### Attribute: `color`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: (border) Border color.
Hexadecimal value or supported named color.
For a hexadecimal value, use the form "##xxxxxx" or "xxxxxx". (required)

### Attribute: `destination`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Absolute or relative pathname where the image output is written.
The image format is determined by the file extension.
The convert and write actions require a destination.
The border, captcha, resize, and rotate actions require either a name attribute or a destination attribute.
You can specify both.
Scorpio supports only CAPTCHA images in PNG format.
If you do not enter a destination, the CAPTCHA image is placed inline in the HTML output and displayed in the web browser.

### Attribute: `difficulty`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `low`
- **Description**: Level of complexity of the CAPTCHA text.

### Attribute: `fontSize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Font size of the text in the CAPTCHA image.
The value must be an integer.

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Format of the image displayed in the browser.
If you do not specify a format, the image is displayed in PNG format.
You cannot display a GIF image in a browser.
GIF images are displayed in PNG format.

### Attribute: `height`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Height in pixels of the image.
For the resize attribute, you also can specify the height as a percentage (an integer followed by the "%" symbol).
The value must be an integer.

### Attribute: `isBase64`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Specifies whether the source is a Base64 string or not.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the ColdFusion image variable to create.
The read action requires name attribute.
The border, resize, and rotate options require a name attribute or a destination attribute.
You can specify both.

### Attribute: `overwrite`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Valid only if the destination attribute is specified.
If the destination file already exists, ColdFusion generates an error if the overwrite option is not set to yes.

### Attribute: `quality`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0.75`
- **Description**: Quality of the JPEG destination file.
Applies only to files with an extension of JPG or JPEG.
Valid values are fractions that range from 0 through 1
(the lower the number, the lower the quality).

### Attribute: `source`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: URL of the source image; for example, "http://www.google.com/ images/logo.gif"
Absolute or relative pathname of the source image; for example, "c:\wwwroot\images\logo.jpg"
ColdFusion image variable containing another image, BLOB, or byte array; for example, "#myImage#" 
Base64 string; for example, "data:image/jpg;base64,/9j/ 4AAQSkZJRgABAQA.............."

### Attribute: `structName`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the ColdFusion structure to be created.

### Attribute: `text`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text string displayed in the CAPTCHA image.
Use capital letters for better readability.

### Attribute: `thickness`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: Border thickness in pixels.
The border is added to the outside edge of the source image,
increasing the image area accordingly.
The value must be an integer.

### Attribute: `width`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Width in pixels of the image.
For resize, you also can specify the width as a percentage
(an integer followed by the "%" symbol).
The value must be an integer.

### Attribute: `fonts`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: One or more valid fonts to use for the CAPTCHA text. Separate multiple fonts with commas. ColdFusion supports only the system fonts that the JDK can recognize.

### Attribute: `interpolation`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `highestQuality`
- **Description**: CF10+ Used when action=resize determines the interpolation algorithm to use.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

