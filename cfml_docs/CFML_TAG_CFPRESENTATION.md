# Tag Name: `cfpresentation`

## Description
Defines the look and feel of a dynamic slide presentation.
 Use the cfpresentation tag as the parent tag for one or more cfpresentationslide tags,
 where you define the content for the presentation.

## Syntax
```cfml
<cfpresentation title="">
```

## Attributes / Variants

### Attribute: `title`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specifies the title of the presentation.

### Attribute: `backgroundColor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `0x727971`
- **Description**: Specifies the background color of the presentation.
 The value is hexadecimal: use the form "##xxxxxx" or "##xxxxxxxx",
 where x = 0-9 or A-F; use two number signs or none.

### Attribute: `control`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `normal`
- **Description**: Specifies the presentation control:

### Attribute: `controlLocation`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `right`
- **Description**: Specifies the location of the presentation control:

### Attribute: `directory`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the directory where the presentation is saved.
 This can be absolute path or a path relative to the CFM page.
 ColdFusion automatically generates the files necessary to
 run the presentation, including:
 index.htm
 components.swf
 loadflash.js
 viewer.swf
 ColdFusion stores any data files in the presentation,
 including images, video clips, and SWF files referenced by the
 cfpresentationslide tags in a subdirectory called data.
 To run the presentation, open the index.htm file.
 If you do not specify a directory, the presentation
 runs in the client browser.

### Attribute: `glowColor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `0x35D334`
- **Description**: Specifies the color used for glow effects on the buttons.
 The value is hexadecimal: use the form "##xxxxxx" or "##xxxxxxxx",
 where x = 0-9 or AF; use two number signs or none.

### Attribute: `initialTab`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `outline`
- **Description**: Specifies which tab will be on top when the presentation is displayed.
 This applies only when the control value is normal:

### Attribute: `lightColor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `0x4E5D60`
- **Description**: Specifies the light color used for light-and shadow effects.
 The value is hexadecimal: use the form "##xxxxxx" or "##xxxxxxxx",
 where x = 0-9 or A-F; use two number signs or none.

### Attribute: `overwrite`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `yes`
- **Description**: Specifies whether files in the directory are overwritten.
 Specify this attribute only when the you specify the directory.
 yes: overwrites files if they are already present
 no: create new files

### Attribute: `primaryColor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `0x6F8488`
- **Description**: Specifies the primary color of the presentation.
 The value is hexadecimal: use the form "##xxxxxx" or "##xxxxxxxx",
 where x = 0-9 or AF; use two number signs or none.

### Attribute: `shadowColor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `0x000000`
- **Description**: Specifies the shadow color used for light-and shadow effects.
 The value is hexadecimal: use the form "##xxxxxx" or "##xxxxxxxx",
 where x = 0-9 or A-F; use two number signs or none.

### Attribute: `showNotes`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `no`
- **Description**: Specifies whether the notes tab is present:

### Attribute: `showOutline`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `yes`
- **Description**: Specifies whether the outline is present:

### Attribute: `showSearch`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `yes`
- **Description**: Specifies whether the search tab is present:

### Attribute: `textColor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `0xFFFFFF`
- **Description**: Specifies the color for all the text in the presentation user interface.

### Attribute: `authpassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Sends a password to the target URL for Basic Authentication. Combined with username to form a base64 encoded string that is passed in the Authenticate header.

### Attribute: `authuser`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Sends a user name to the target URL for Basic Authentication. Combined with password to form a base64 encoded string that is passed in the Authenticate header.

### Attribute: `autoplay`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Specifies whether to play the presentation automatically:
 * true: the presentation automatically runs through the entire presentation at startup.
 * false: the user must click the Play button to start the presentation and click the Next button to advance to the next slide in the presentation.

### Attribute: `loop`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Specifies whether the presentation runs in a loop:
 * true: the presentation restarts automatically after it ends.
 * false: the user must click the Play button to restart the presentation.

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

### Attribute: `proxyhost`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Host name or IP address of a proxy server to which to send the request.

### Attribute: `proxyport`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The port to connect to on the proxy server.

### Attribute: `useragent`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text to put in the HTTP User-Agent request header field. Used to identify the request client software.

### Attribute: `destination`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Destination directory

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies file format for conversion. The `flashpaper` format has been deprecated since CF11+

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

