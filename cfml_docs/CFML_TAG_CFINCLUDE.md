# Tag Name: `cfinclude`

## Description
Includes the content from the referenced file (template). The content may be executed as CFML, see compatibility info below. You can embed cfinclude tags recursively. For another way to encapsulate CFML, see cfmodule. (A CFML page was formerly sometimes called a CFML template or a template.)

## Syntax
```cfml
<cfinclude template="" runonce="true|false">
```

## Attributes / Variants

### Attribute: `template`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A logical path to a CFML page.

### Attribute: `runonce`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**:  CF10+ If set to true, the given template is not processed again for a given request if it has already been processed.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

