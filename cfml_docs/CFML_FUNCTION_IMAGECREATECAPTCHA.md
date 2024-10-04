# Function Name: `ImageCreateCaptcha`

## Description
 Create a Completely Automated Public Turing test to tell Computers and Humans Apart (CAPTCHA) image, a distorted text image that is human-readable, but not machine-readable, used in a challenge-response test for preventing spam.

## Return Type
`any`

## Syntax
```cfml
imageCreateCaptcha (height, width, text [, difficulty, fonts, fontsize)
```

## Arguments

### Argument: `height`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Height in pixels of the image.

### Argument: `width`
- **Type**: `numeric`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Width in pixels of the image.

### Argument: `text`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Text string displayed in the CAPTCHA image. Use capital letters for better readability. Do not include spaces because users cannot detect them in the resulting CAPTCHA image..

### Argument: `difficulty`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Level of complexity of the CAPTCHA text. Specify one of the following levels of text distortion: low, medium, and high

### Argument: `font`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: One or more valid fonts to use for the CAPTCHA text. Separate multiple fonts with commas. ColdFusion supports only the system fonts that the JDK can recognize. For example, TTF fonts in the Windows directory are supported on Windows.

### Argument: `fontsize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Font size of the text in the CAPTCHA image. The value must be an integer.

## Limitations and Other Info

- **Related Functions**: `cfimage`, `imagecaptcha`
- **Coldfusion Support**: Minimum version: `10`.

