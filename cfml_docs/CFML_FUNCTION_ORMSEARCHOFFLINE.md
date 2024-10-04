# Function Name: `ORMSearchOffline`

## Description
 Performs search on the indexed properties but returns only the stored fields. For this function to work, specify indexStore=true on the properties on which you want to perform the search.

## Return Type
`struct`

## Syntax
```cfml
ormSearchOffline(query_text, entityName, fields_to_be_selected)
ormSearchOffline(query_text, entityName, fields_to_be_selected, fields)
ormSearchOffline(query_text, entityName, fields_to_be_selected, fields, optionMap);
```

## Arguments

### Argument: `query_text`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The text to be searched for or a complete Lucene query. For details of Lucene query, see http://lucene.apache.org/core/old_versioned_docs/versions/.

### Argument: `entityName`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the entity to be searched.

### Argument: `fields_to_be_selected`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Fields to be returned as keys in the resultant struct.

### Argument: `fields`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Fields in which search has to be performed.

### Argument: `optionMap`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: 

### Argument: `extra options`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**:  can be passed while executing Lucene query. The options can be: sort, offset, maxResults

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `10`.

