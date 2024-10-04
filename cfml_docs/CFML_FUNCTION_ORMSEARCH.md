# Function Name: `ORMSearch`

## Description
Searches for given text in specific properties or entities.

## Return Type
`struct`

## Syntax
```cfml
ormSearch('query_text', 'entityName')
ormSearch('query_text', 'entityName', fields)
ormSearch('query_text', 'entityName', fields, optionMap);
```

## Arguments

### Argument: `query_text`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The text to be searched for or a complete Lucene query.In the case of ORMSearch('query_text', 'entityName'), only Lucene query is supported. For details of Lucene query, see http://lucene.apache.org/core/old_versioned_docs/versions/3_0_0/queryparsersyntax.html

### Argument: `entityName`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the entity to be searched.

### Argument: `fields`
- **Type**: `array`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Fields in which search has to be performed. This can be an array of strings. If you are performing a Lucene query, you need not specify this field. In other words, if you do not specify this value, a Lucene query is performed. Field name is case-sensitive.

### Argument: `optionMap`
- **Type**: `struct`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Extra options that can be passed while executing Lucene query. The options are: Sort, Offset, maxResults

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `10`.

