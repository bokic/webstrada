# Tag Name: `cfmediaplayer`

## Description
Creates an in-built media player that lets you play FLV files

## Syntax
```cfml
<cfmediaplayer source="">
```

## Attributes / Variants

### Attribute: `width`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Width of the media player, in pixels.

### Attribute: `height`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Height of the media player, in pixels.

### Attribute: `fullscreencontrol`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether full screen is enabled

### Attribute: `hideBorder`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value that specifies if you want to hide border for the media player panel

### Attribute: `controlbar`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value that specifies if you want to display the control panel for the media player

### Attribute: `align`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the vertical alignment of the media player.

### Attribute: `onload`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to run on loading the player component.

### Attribute: `bgcolor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The background color of the media player specified as a Hexadecimal value without a "#"

### Attribute: `source`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The URL to the FLV file.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the media player.
The name attribute is required when you invoke JavaScript functions.

### Attribute: `quality`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The quality of the media playback

### Attribute: `hideTitle`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value that specifies if you want to hide title for the media player panel

### Attribute: `oncomplete`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to run when the FLV file has finished playing.

### Attribute: `onstart`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom JavaScript function to run when the FLV file starts playing.

### Attribute: `wmode`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the absolute positioning and layering capabilities in your browser

### Attribute: `autoPlay`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value that specifies if the media player should automatically play the FLV file on loading the CFM page.

### Attribute: `style`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify style for mediaplayer

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

