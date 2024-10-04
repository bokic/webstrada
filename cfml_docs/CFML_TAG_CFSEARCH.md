# Tag Name: `cfsearch`

## Description
Searches Verity collections using CFML or K2Server, whichever search engine a collection is registered by.
 (CFML can also search collections that have not been
 registered, with the cfcollection tag.)

 A collection must be created and indexed before this tag can
 return search results.

## Syntax
```cfml
<cfsearch name="" collection="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the search query.

### Attribute: `collection`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: One or more collection names. You can specify more
 than one collection unless you are performing a
 category search (that is, specifying category or
 categoryTree).

 One or more collection names. You can specify more
 than one collection unless you are performing a category search (that is, specifying category or categoryTree).

### Attribute: `category`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A list of categories, separated by commas, to which
 the search is limited. If specified, and the collection
 does not have categories enabled, ColdFusion

 throws an exception.

### Attribute: `categorytree`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The location in a hierarchical category tree at which
 to start the search. ColdFusion searches at and
 below this level. If specified, and the collection does
 not have categories enabled, ColdFusion throws an
 exception. Can be used in addition to category
 attribute.

### Attribute: `status`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the name of the structure variable into
 which ColdFusion places search information, including
 alternative criteria suggestions (spelling corrections).

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `simple`
- **Description**: Used to specify the parser that Verity/SOLR uses to process
 the criteria.

### Attribute: `criteria`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Search criteria. Follows the syntax rules of the type
 attribute. If you pass a mixed-case entry in this attribute,
 the search is case-sensitive. If you pass all uppercase or
 all lowercase, the search is case-insensitive. Follow
 Verity syntax and delimiter character rules; see Using Verity Search Expressions in Developing CFML MX Applications.

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `all`
- **Description**: Maximum number of rows to return in query results.
 Default: all

### Attribute: `startrow`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: First row number to get.
 Default: 1

### Attribute: `suggestions`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `never`
- **Description**: Specifies whether Verity/SOLR returns spelling suggestions
 for possibly misspelled words.

### Attribute: `contextPassages`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `3`
- **Description**: The number of passages/sentences Verity returns in
 the context summary (that is, the context column of
 the results).
 Default: 3

### Attribute: `contextBytes`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `300`
- **Description**: The maximum number of bytes Verity returns in the
 context summary.
 Default: 300

### Attribute: `contextHighlightBegin`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `<b>`
- **Description**: The HTML to prepend to search terms in the context
 summary. Use this attribute in conjunction with
 contextHighlightEnd to highlight search terms in the
 context summary.

### Attribute: `contextHighlightEnd`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `</b>`
- **Description**: The HTML to prepend to search terms in the context
 summary. Use this attribute in conjunction with
 contextHighlightEnd to highlight search terms in the
 context summary.

### Attribute: `previousCriteria`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a result set from an existing set of search
 results. Verity searches the result set for criteria
 without regard to the previous search score or rank.
 Use this attribute to implement searching within result
 sets.

### Attribute: `language`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Deprecated. This attribute is now ignored and the language of the collection is used to perform the search.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

