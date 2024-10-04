# Function Name: `HTMLCodeFormat`

## Description
Replaces special characters in a string with their HTML-escaped equivalents and inserts &lt;pre&gt; and &lt;/pre&gt; tags at the beginning and end of the string.
The only difference between this function and `HTMLEditFormat` is that `HTMLEditFormat` doesn't surround the text in HTML `pre` tags.

## Return Type
`string`

## Syntax
```cfml
htmlCodeFormat(string [, version])
```

## Arguments

### Argument: `string`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A String or variable that contains one.

### Argument: `version`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `2.0`
- **Description**: HTML version to use. Currently ignored.
 -1: The latest implementation of HTML
 2.0: HTML 2.0 (Default)
 3.2: HTML 3.2

## Limitations and Other Info

- **Related Functions**: `htmlEditFormat`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

