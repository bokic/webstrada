# Tag Name: `cftable`

## Description
Builds a table in a CFML page. This tag renders data as preformatted text, or, with the HTMLTable attribute, in an HTML table. If you do not want to write HTML table tag code, or if your data can be presented as preformatted text, use this tag. Preformatted text (defined in HTML with the &lt;pre&gt; and &lt;/pre&gt; tags) displays text in a fixed-width font. It displays white space and line breaks exactly as they are written within the pre tags. For more information, see an HTML reference guide.

 To define table column and row characteristics, use the cfcol tag within this tag.

## Syntax
```cfml
<cftable query="">
```

## Attributes / Variants

### Attribute: `query`
- **Type**: `query`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of cfquery from which to draw data.

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum number of rows to display in the table.

### Attribute: `colSpacing`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Number of spaces between columns

### Attribute: `headerLines`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Number of lines to use for table header (the default leaves
 one line between header and first row of table).

### Attribute: `htmltable`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Renders data in an HTML 3.0 table.

 If you use this attribute (regardless of its value),
 CFML renders data in an HTML table.

### Attribute: `border`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Displays border around table.

 If you use this attribute (regardless of its value),
 CFML displays a border around the table.

 Use this only if you use the HTMLTable attribute.

### Attribute: `colheaders`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Displays column heads. If you use this attribute, you must
 also use the cfcol tag header attribute to define them.

 If you use this attribute (regardless of its value),
 CFML displays column heads.

### Attribute: `startrow`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The query result row to put in the first table row.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

