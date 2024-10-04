# Tag Name: `cfxml`

## Description
Creates a CFML XML document object that contains the
 markup in the tag body. This tag can include XML and CFML tags.
 CFML processes the CFML code in the tag body, then assigns
 the resulting text to an XML document object variable.

## Syntax
```cfml
<cfxml variable="">
```

## Attributes / Variants

### Attribute: `variable`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of an XML variable

### Attribute: `casesensitive`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Maintains the case of document elements and attributes

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

