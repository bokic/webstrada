# Tag Name: `cfoutput`

## Description
Displays output that can contain the results of processing CFML variables and functions. You can use the `query` attribute to loop over the result set of a database query.

## Syntax
```cfml
<cfoutput>
```

## Attributes / Variants

### Attribute: `query`
- **Type**: `query`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of cfquery from which to draw data for output section.

### Attribute: `group`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Query column to use to group sets of records. Eliminates
 adjacent duplicate rows when data is sorted. Use if you
 retrieved a record set ordered on one or more a query
 columns. For example, if a record set is ordered on
 "Customer_ID" in the cfquery tag, you can group the output
 on "Customer_ID."

### Attribute: `groupcasesensitive`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Whether to consider the case in grouping rows.

### Attribute: `startrow`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Row from which to start output. Only considered when the query attribute is set.
This attribute in combination with maxrows can be used to create some paging.

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum number of rows to display. Only considered when the query attribute is set.

### Attribute: `encodefor`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF2016+ Lucee5.1+ When set applies an encoder to all variables to prevent XSS. For example if you specify `html` each variable will be wrapped by a call to the `encodeForHTML` function.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

