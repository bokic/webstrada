# Tag Name: `cfpdfparam`

## Description
Provides additional information for the cfpdf tag.
 The cfpdfparam tag applies only to the merge action of the cfpdf tag.
 The cfpdfparam tag is always a child tag of the cfpdf tag.

## Syntax
```cfml
<cfpdfparam source="">
```

## Attributes / Variants

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specify the user or owner password, if the source PDF file is passwordprotected. (optional)

### Attribute: `source`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specify the source PDF file to merge. (optional)

### Attribute: `pages`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Page or pages of the PDF source file to merge. You can specify a range of pages, for example, "1-5 ", or a comma-separated list of pages, for example, "1-5,9-10,18".

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

