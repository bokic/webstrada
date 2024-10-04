# Tag Name: `cfmodule`

## Description
Invokes a custom tag for use in CFML application pages.
 This tag processes custom tag name conflicts.

## Syntax
```cfml
<cfmodule>
```

## Attributes / Variants

### Attribute: `template`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Mutually exclusive with the name attribute. A path to the
 page that implements the tag.

 Relative path: expanded from the current page
 Absolute path: expanded using CFML mapping
 A physical path is not valid.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Mutually exclusive with the template attribute. A custom
 tag name, in the form "Name.Name.Name..." Identifies
 subdirectory, under the CFML tag root directory,
 that contains custom tag page. 
For example (Windows format):

 `cfmodule name = "superduper.Forums40.GetUserOptions"` 

 This identifies the page GetUserOptions.cfm in the
 directory CustomTags\superduper\Forums40 under the
 CFML root directory.

### Attribute: `attributecollection`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A collection of key-value pairs that represent
 attribute names and values. You can specify multiple
 key-value pairs. You can specify this attribute only
 once.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

