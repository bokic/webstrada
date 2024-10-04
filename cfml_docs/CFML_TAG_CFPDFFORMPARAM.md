# Tag Name: `cfpdfformparam`

## Description
Provides additional information to the cfpdfform tag.
 The cfpdfformparam tag is always a child tag of the cfpdfform or cfpdfsubform tag.
 Use the cfpdfformparam tag to populate fields in a PDF form.

## Syntax
```cfml
<cfpdfformparam name="" value="">
```

## Attributes / Variants

### Attribute: `index`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the index associated with the field name.
 If multiple fields have the same name, the index
 value is used to locate one of them. (optional, default=1)

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The field name on the PDF form. (required)

### Attribute: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The value associated with the field name.
 For interactive fields, specify a ColdFusion variable. (optional)

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

