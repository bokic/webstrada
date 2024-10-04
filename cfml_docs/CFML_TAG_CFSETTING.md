# Tag Name: `cfsetting`

## Description
Controls aspects of page processing, such as the output of content outside cfoutput tags.

## Syntax
```cfml
<cfsetting enablecfoutputonly=true|false>
```

## Attributes / Variants

### Attribute: `enablecfoutputonly`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: true: Blocks output of content that is outside cfoutput tags.
false: Displays content that is outside cfoutput tags.

### Attribute: `showdebugoutput`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: true: If debugging is enabled in the Administrator, displays
 debugging information
 false: suppresses debugging information that would otherwise
 display at end of generated page.

### Attribute: `requesttimeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Integer; number of seconds. Time limit, after which
 CFML processes the page as an unresponsive thread.
 Overrides the timeout set in the CFML Administrator.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

