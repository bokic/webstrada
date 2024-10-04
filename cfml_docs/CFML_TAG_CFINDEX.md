# Tag Name: `cfindex`

## Description
Populates a Verity search engine collection with an index of
 documents on a file system or of CFML query result sets.

 A collection must exist before it can be populated.

## Syntax
```cfml
<cfindex collection="">
```

## Attributes / Variants

### Attribute: `collection`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of a collection that is registered by CFML; for
 example, "personnel"
 Name and absolute path of a collection that is not
 registered by CFML; for example:
 "e:\collections\personnel"

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: - update: updates a collection and adds key to the index.
 - delete: removes collection documents as specified by
 the key attribute.
 - purge: deletes all of the documents in a collection.
 Causes the collection to be taken offline, preventing
 searches.
 - refresh: deletes all of the documents in a collection,
 and then performs an update.

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: file: using the key attribute value of the query result as
 input, applies action value to filenames or filepaths.
 path: using the key attribute value of the query result as
 input, applies action to filenames or filepaths that
 pass the extensions filter
 custom: If action = "update" or "delete": applies action to
 custom entities in query results.

### Attribute: `title`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: * Title for collection
 * Query column name for type and a valid query name
 Permits searching collections by title or displaying a
 separate title from the key

### Attribute: `key`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: * Absolute path and filename, if type = "file"
 * Absolute path, if type = "path"
 * A query column name (typically, the primary key column
 name), if type = "custom"
 * A query column name, if type = any other value

 This attribute is required for the actions listed, unless
 you intend for its value to be an empty string.

### Attribute: `body`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: * ASCII text to index
 * Query column name(s), if name is specified in query

 You can specify columns in a delimited list. For example:
 "emp_name, dept_name, location"

### Attribute: `custom1`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom field in which you can store data during an indexing
 operation. Specify a query column name for type, and a
 query name.

### Attribute: `custom2`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom field in which you can store data during an indexing
 operation. Specify a query column name for type, and a
 query name.

### Attribute: `custom3`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom field in which you can store data during an indexing
 operation. Specify a query column name for type, and a
 query name. (Added in ColdFusion 7)

### Attribute: `custom4`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Custom field in which you can store data during an indexing
 operation. Specify a query column name for type, and a
 query name. (Added in ColdFusion 7)

### Attribute: `category`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A string value that specifies one or more search categories
 for which to index the data. You can define multiple
 categories, separated by commas, for a single index.

### Attribute: `categoryTree`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A string value that specifies a hierarchical category or
 category tree for searching. It is a series of categories
 separated by forward slashes ("/"). You can specify only
 one category tree.

### Attribute: `urlpath`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If type="file" or "path", specifies the URL path. When the
 collection is searched with cfsearch, this pathname is
 prefixed to filenames and returned as the url attribute.

### Attribute: `extensions`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Delimited list of file extensions that CFML uses to
 index files, if type = "Path".
 "*." returns files with no extension.

 For example: the following code returns files with a
 listed extension or no extension:
 extensions = ".htm, .html, .cfm, .cfml, "*."

### Attribute: `query`
- **Type**: `query`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Query against which collection is generated

### Attribute: `recurse`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: if type = "path", directories below the path
 specified in key are included in indexing operation

### Attribute: `language`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `english`
- **Description**: For options, see cfcollection. Requires the appropriate
 Verity Locales language pack (Western Europe, Asia,
 Multilanguage, Eastern Europe/Middle Eastern).

### Attribute: `status`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of the structure into which ColdFusion MX
 returns status information.

### Attribute: `prefix`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies the location of files to index when the computer that contains the K2 Search Service is not the computer on which you installed ColdFusion, and when you index files with the type attribute set to path.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

