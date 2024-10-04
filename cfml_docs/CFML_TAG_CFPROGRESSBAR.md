# Tag Name: `cfprogressbar`

## Description
Creates progressbar

## Syntax
```cfml
<cfprogressbar name="">
```

## Attributes / Variants

### Attribute: `width`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Width in pixel. Default='300'

### Attribute: `height`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Height in pixel.

### Attribute: `bind`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A bind expression specifying a client JavaScript function or server CFC that the control calls to get progress information each time the period defined by the
interval attribute elapses. You cannot use this attribute with a duration attribute.

### Attribute: `oncomplete`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: This function will be called by CF once work is done. This can be used to reset status of progress bar text or enabling the button, which might be in disable mode while task was under progress or something more.

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the progressBar.

### Attribute: `duration`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If callback is not defined then based on this value, ColdFusion will decide percentage work done. One of the callback or totaltime attribute needs to be defined.

### Attribute: `interval`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Time interval on which progressbar will keep on updating.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The following are the supported styles: 
bgcolor: The background color for the progress bar. A hexadecimal value without # prefixed.
textcolor: Text color on progress bar.
progresscolor: Color used to indicate the progress

### Attribute: `autoDisplay`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Set to true to display the progress bar.

### Attribute: `onError`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The JavaScript function to run on an error condition. The error can be a network error or server-side error.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

