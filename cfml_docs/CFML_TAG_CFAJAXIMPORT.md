# Tag Name: `cfajaximport`

## Description
Controls the JavaScript files that are imported for use on pages that use ColdFusion AJAX 
 tags and features.

## Syntax
```cfml
<cfajaximport>
```

## Attributes / Variants

### Attribute: `scriptsrc`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the URL, relative to the web root, of the 
 directory that contains the JavaScript files 
 used by ColdFusion. When you use this attribute, 
 the specified directory must have the same 
 structure as the /CFIDE/scripts directory.

### Attribute: `csssrc`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the URL, relative to the web root, of the 
 directory that contains the CSS files used by 
 ColdFusion AJAX features, with the exception of 
 the rich text editor. This directory must have the 
 same directory structure, and contain the same 
 CSS files, and image files required by the CSS 
 files, as the 
 web_root/CFIDE/scripts/ajax/resources 
 directory. 
 This attribute lets you create different custom 
 styles for ColdFusion AJAX controls in different 
 applications.

### Attribute: `tags`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A comma-delimited list of tags or tag-attribute 
 combinations for which to import the supporting 
 JavaScript files on this page.

### Attribute: `attributecollection`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: You can specify this tag's attributes in an attributeCollection whose value is a 
 structure. Specify the structure name in the attributeCollection and use the tag 
 attribute names as structure keys.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

