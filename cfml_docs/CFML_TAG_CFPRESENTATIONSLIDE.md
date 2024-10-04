# Tag Name: `cfpresentationslide`

## Description
Creates a slide dynamically from an HTML source file,
 HTML and CFML code, or an SWF source file.
 If you do not specify a source file, you must include the HTML or CFML code for
 the body of the slide within the cfpresentationslide tag. The cfpresentationslide
 is the child tag of the cfpresentation tag.

## Syntax
```cfml
<cfpresentationslide>
```

## Attributes / Variants

### Attribute: `title`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specifies the title of the slide.

### Attribute: `audio`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the path of the audio file relative to the CFM page.
 The audio file must be an MP3 file.
 A slide cannot have both audio and video specified

### Attribute: `bottomMargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Specifies the bottom margin of the slide.

### Attribute: `duration`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the duration in seconds that the slide is played. (required)

### Attribute: `notes`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the notes used for the slide.

### Attribute: `presenter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the presenter of the slide.
 A slide can have only one presenter.
 This name must match one of the presenter names in the cfpresenter tag.
 If no presenter is specified, it will take the first presenter
 specified in the presenter list.

### Attribute: `rightMargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Specifies the right margin of the slide.

### Attribute: `scale`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1.0`
- **Description**: Specifies the scale used for the HTML content on the slide
 presentation. If the scale attribute is not specified and
 the content cannot fit in one slide, it will automatically
 be scaled down to fit on the slide.

### Attribute: `src`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: HTML or SWF source files used as a slide. You can specify
 the following as the slide source:
 an absolute path
 a path relative to the CFM page
 a URL: you can specify the URL only if the source is an HTML file
 SWF files must be present on the system running ColdFusion and the path
 must either be an absolute path or path relative to the CFM page.
 If the src value is not specified, you must specify HTML/CFML code
 as the body. If you specify a source file and HTML /CFML, ColdFusion
 ignores the source file and displays the HTML/CFML in the slide.

### Attribute: `topMargin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `0`
- **Description**: Specifies the top margin of the slide.

### Attribute: `video`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the video file used for the presenter of the slide.
 If you specify video for the slide and an image for the presenter,
 the video is used instead of the image for the slide. You cannot specify
 both audio and video for a slide. The video must be an FLV file.

### Attribute: `advance`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides the cfpresentation tag autoPlay attribute for the slide:
 * auto: after the slide plays, the presentation advances to the next slide automatically. This is the default value if cfpresentation autoPlay="yes".
 * never: after the slide plays, the presentation does not advance to the next slide until the user clicks the Next button. This is the default value if cfpresentation autoPlay="no".
 * click: after the slide plays, the presentation advances to the next slide if the user clicks anywhere in the main presentation area.

### Attribute: `authpassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Use to pass a password to the target URL for Basic Authentication. Combined with username to form a base64 encoded string that is passed in the Authenticate header.

### Attribute: `authuser`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Use to pass a user name to the target URL for Basic Authentication. Combined with password to form a base64 encoded string that is passed in the Authenticate header.

### Attribute: `marginbottom`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Bottom margin of the slide.

### Attribute: `marginleft`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Left margin of the slide.

### Attribute: `marginright`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Right margin of the slide.

### Attribute: `margintop`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Top margin of the slide.

### Attribute: `useragent`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text to put in the HTTP User-Agent request header field. Identifies the request client software.

### Attribute: `slides`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Used to specify the slide numbers required to export from ppt file

## Limitations

- **Must be nested inside**: `cfpresentation`
- **Must not be nested inside**: *None*

