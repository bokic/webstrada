# Tag Name: `cfpdfsubform`

## Description
Populates a subform within the cfpdfform tag.
 The cfpdfsubform tag can be a child tag of the cfpdfform tag
 or nested in another cfpdfsubform tag.

## Syntax
```cfml
<cfpdfsubform name="">
```

## Attributes / Variants

### Attribute: `index`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Index associated with the field name.
 If multiple fields have the same name, ColdFusion
 uses the index value is to locate one of them. (optional, default=1)

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the subform corresponding to subform name in the PDF form. (required)

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

