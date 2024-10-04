# Tag Name: `cffileupload`

## Description
Ajax File upload

## Syntax
```cfml
<cffileupload url="">
```

## Attributes / Variants

### Attribute: `url`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The URL to the server where the files are uploaded.

### Attribute: `width`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Width of the file upload control, in pixels.

### Attribute: `title`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Title for the upload dialog.

### Attribute: `extensionfilter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Use this attribute to specify the type of file that you will allow to be uploaded. 
For example, to let only image files to be uploaded, you can specify file extensions like .jpg, .jpeg, or .png

### Attribute: `uploadbuttonlabel`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Label of the Upload button.

### Attribute: `progressbar`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to display Progress Bar or not. Default true

### Attribute: `height`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Height of the file upload control, in pixels.

### Attribute: `maxUploadSize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The maximum file size, in megabytes, allowed for the upload. Default 10MB.

### Attribute: `maxFileSelect`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The maximum number of files allowed for the upload.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A CSS style specification that defines layout styles

### Attribute: `bgcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The background color for the file upload control. A hexadecimal value without &quot;#&quot; prefixed.

### Attribute: `addButtonLabel`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Label of the Add button

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the file upload component.

### Attribute: `oncomplete`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The JavaScript function to run after the file upload completes

### Attribute: `onerror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The JavaScript function to run on an error condition. The error can be a network error or server-side error

### Attribute: `clearButtonLabel`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Label of the Clear button

### Attribute: `deleteButtonLabel`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Label of the Delete button

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the default alignment

### Attribute: `wmode`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the absolute positioning and layering capabilities in your browser:

### Attribute: `stopOnError`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies whether or not to ignore the exceptions for this operation. When the value is
true, it stops uploading, displays an appropriate error

### Attribute: `hideUploadButton`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value that specifies if the Upload button should appear in the media player

### Attribute: `onUploadComplete`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The JavaScript function to run after the all uploads have completed

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

