# Tag Name: `cfcollection`

## Description
Creates, registers, and administers Verity search engine
 collections.

 A collection that is created with the cfcollection tag is
 internal. A collection created any other way is external.

 A collection that is registered with CFML using the
 cfcollection tag or registered with the K2 Server by editing
 the k2server.ini file is registered. Other collections are
 unregistered.

## Syntax
```cfml
<cfcollection action="categorylist">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: `list`
- **Description**: categorylist: retrieves categories from the collection and
 indicates how many documents are in each one. Returns
 a structure of structures in which the category
 representing each substructure is associated with a
 number of documents. For a category in a category tree,
 the number of documents is the number at or below that
 level in the tree.
 create: registers the collection with CFML.
 - If the collection is present: creates a map to it
 - If the collection is not present: creates it
 delete: unregisters a collection.
 - If the collection was registered with action = create:
 deletes its directories
 - If the collection was registered and mapped: does not
 delete collection directories
 optimize: optimizes the structure and contents of the
 collection for searching; recovers space.
 list: returns a query result set, named from the name
 attribute value, of the attributes of the collections
 that are registered by CFML and K2 Server.
 map: creates a map to the collection. It is not necessary
 to specify this value. Deprecated in CF7.
 repair: fixes data corruption in a collection. Deprecated in CF7.

### Attribute: `collection`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A collection name. The name can include spaces

### Attribute: `path`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Absolute path to a Verity/Lucene/SOLR collection.

### Attribute: `language`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `english`
- **Description**: Options are listed in Usage section. Requires the
 appropriate (European or Asian) Verity Locales language
 pack.

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name for the query results returned by the list action.

### Attribute: `categories`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Used only for creating a collection:
 - true: This collection includes support for categories.
 - false: This collection does not support categories. Default.

### Attribute: `engine`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `verity`
- **Description**: Search engine

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

