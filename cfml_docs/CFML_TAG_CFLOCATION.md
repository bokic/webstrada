# Tag Name: `cflocation`

## Description
Stops execution of the current page and redirects to the specified URI.

## Syntax
```cfml
<cflocation url="page.cfm" addtoken="false" statusCode="301">
```

## Attributes / Variants

### Attribute: `url`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: URI or URL to redirect to.

### Attribute: `addtoken`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `true`
- **Description**: Appends the CFID, CFTOKEN, JSESSIONID and possibly other session/client identifiers to the URL. Security best practices recommend setting this to false.

### Attribute: `statuscode`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `302`
- **Description**: The 30X HTTP status code. CF8+

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

