# Tag Name: `cftimer`

## Description
Displays execution time for a specified section of
 CFML code. ColdFusion MX displays the timing information
 along with any output produced by the timed code.

## Syntax
```cfml
<cftimer>
```

## Attributes / Variants

### Attribute: `label`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: ` `
- **Description**: Label to display with timing information.
 Default: " "

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `debug`
- **Description**: - inline: displays timing information inline, following the
 resulting HTML.
 - outline: displays timing information and also displays a line
 around the output produced by the timed code. The browser
 must support the FIELDSET tag to display the outline.
 - comment: displays timing information in an HTML comment
 in the format <!-- label: elapsed-time ms -->. The default label
 is cftimer.
 - debug: displays timing information in the debug output
 under the heading CFTimer Times.
 Default: debug

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

