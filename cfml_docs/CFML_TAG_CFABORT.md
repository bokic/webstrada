# Tag Name: `cfabort`

## Description
Stops the processing of a CFML page at the tag location.
 CFML returns everything that was processed before the
 tag. The tag is often used with conditional logic to stop
 processing a page when a condition occurs.

## Syntax
```cfml
<cfabort>
```

## Attributes / Variants

### Attribute: `showerror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Error to display, in a standard CFML error page,
 when tag executes

### Attribute: `attributecollection`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: You can specify this tag's attributes in an attributeCollection whose value is a 
 structure. Specify the structure name in the attributeCollection and use the tag‚ 
 attribute names as structure keys.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

