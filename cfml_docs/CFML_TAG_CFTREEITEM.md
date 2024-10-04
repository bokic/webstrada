# Tag Name: `cftreeitem`

## Description
Populates a form tree control, created with the cftree tag,
 with elements. To display icons, you can use the img values
 that CFML provides, or reference your own icons.

## Syntax
```cfml
<cftreeitem>
```

## Attributes / Variants

### Attribute: `bind`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A bind expression specifying a CFC of JavaScript
 function that dynamically gets all tree nodes. You can use
 this attribute only at the top level of the tree, and in this
 case, the tree can have only cftreeitem tag.
 If you use the bind attribute, the only other allowed
 attribute is onBindError. For details creating trees that
 using binding, see "Using HTML format trees" in Chapter
 33, "Using AJAX UI Components and Features" in
 ColdFusion Developer's Guide

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Value passed when cfform is submitted. When populating a
 tree with data from a cfquery, specify columns in a
 delimited list. Example: value = "dept_id,emp_id"

### Attribute: `display`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Tree item label. When populating a tree with data from a
 query, specify names in a delimited list. Example:
 display = "dept_name,emp_name"

### Attribute: `parent`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Value for tree item parent.

### Attribute: `img`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Image name, filename, or file URL for tree item icon.

 You can specify a custom image. To do so, include path and
 file extension; for example:

 img = "../images/page1.gif"

 To specify more than one image in a tree, or an image at
 the second or subsequent level, use commas to separate
 names, corresponding to level; for example:


 img = "folder,document"
 img = ",document" (example of second level)

### Attribute: `imgopen`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Icon displayed with open tree item. You can specify icon
 filename with a relative path. You can use a CFML
 image.

### Attribute: `href`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: URL to associate with tree item or query column for a tree
 that is populated from a query. If href is a query column,
 its value is the value populated by query. If href is not
 recognized as a query column, it is assumed that its text
 is an HTML href.

 When populating a tree with data from a query, HREFs can be
 specified in delimited list; for example:

 href = "http://dept_svr,http://emp_svr"

### Attribute: `target`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Target attribute of href URL. When populating a tree with
 data from a query, specify target in delimited list:

 target = "FRAME_BODY,_blank"

### Attribute: `query`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Query name to generate data for the treeitem.

### Attribute: `queryAsRoot`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Defines query as the root level. This avoids having to
 create another parent cftreeitem.

 * Yes
 * No
 * String to use as the root name
 If you do not specify a root name, CFML returns the
 query name as the root.

### Attribute: `expand`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Yes: expands tree to show tree item children
 No: keeps tree item collapsed

### Attribute: `onbinderror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a JavaScript function to execute if evaluating a bind expression results in an error. The function must take two attributes: an HTTP status code and a message.

## Limitations

- **Must be nested inside**: `cftree`
- **Must not be nested inside**: *None*

