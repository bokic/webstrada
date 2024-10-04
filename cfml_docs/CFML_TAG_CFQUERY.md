# Tag Name: `cfquery`

## Description
Passes queries or SQL statements to a data source.
 It is recommended that you use the cfqueryparam tag within
 every cfquery tag, to help secure your databases from
 unauthorized users

## Syntax
```cfml
<cfquery>SQL</cfquery>
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of query. Used in page to reference query record set.
 Must begin with a letter. Can include letters, numbers,
 and underscores.

### Attribute: `datasource`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of data source from which query gets data. As of CF9+ you can specify a default datasource in Application.cfc using the variable this.datasource

### Attribute: `timezone`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee4+ the timezone used to convert a date object to a timestamp (string), this value is needed when your database runs in another timezone and you are not using cfqueryparam to insert dates.

### Attribute: `dbtype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Type of source query against which the SQL will be executed. Specify either dbtype or dataSource, not both.  Supports the following values: `query`: for querying an existing query object (i.e. Query of Queries); `hql`: for querying an ORM. NOTE: Supported SQL syntax varies depending on this value.

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides username in data source setup.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides password in data source setup.

### Attribute: `maxrows`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `-1`
- **Description**: Maximum number of rows to return in record set.
 -1 returns all records.

### Attribute: `blockfactor`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum rows to get at a time from server. Range: 1 - 100.
 Might not be supported by some database systems.

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Maximum number of seconds that each action of a query is
 permitted to execute before returning an error. The
 cumulative time may exceed this value.

 For JDBC statements, CFML sets this attribute. For
 other drivers, check driver documentation.

### Attribute: `cachedafter`
- **Type**: `date`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Date value (for example, April 16, 1999, 4-16-99). If date
 of original query is after this date, CFML uses
 cached query data. To use cached data, current query must
 use same SQL statement, data source, query name, user name,
 password.

 A date/time object is in the range 100 AD-9999 AD.

 When specifying a date value as a string, you must enclose
 it in quotation marks.

### Attribute: `cachedwithin`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Timespan, using the CreateTimeSpan function. If original
 query date falls within the time span, cached query data is
 used. CreateTimeSpan defines a period from the present,
 back. Takes effect only if query caching is enabled in the
 Administrator.

 To use cached data, the current query must use the same SQL
 statement, data source, query name, user name, and password.

### Attribute: `debug`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: If debugging is enabled, but the Administrator
 Database Activity option is not enabled, displays SQL
 submitted to datasource and number of records returned
 by query.
 No: If the Administrator Database Activity option is
 enabled, suppresses display.

### Attribute: `result`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF8+ Specifies a name for the structure in which cfquery returns
 the result variables.
 * SQL: The SQL statement that was executed. (string)
 * Cached: If the query was cached. (boolean)
 * SqlParameters: An ordered Array of cfqueryparam values. (array)
 * RecordCount: Total number of records in the query. (numeric)
 * ColumnList: Column list, comma separated. (string)
 * ExecutionTime: Execution time for the SQL request. (numeric)
 * GENERATEDKEY: CF9+ If the query was an INSERT with an identity or auto-increment value the value of that ID is placed in this variable.

### Attribute: `ormoptions`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF9+ A structure of ORM Options when used for HQL queries (9.0.1+). 

### Attribute: `cacheID`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ A value to serve as cache identifier when cachedWithin or cachedAfter are specified.

### Attribute: `cacheRegion`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ The name of the region  cachedWithin or cachedAfter are specified.

### Attribute: `clientInfo`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ A structure containing properties to be set on the database connection.

### Attribute: `fetchClientInfo`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF10+ When true returns a struct with the clientInfo argument value passed by the last query

### Attribute: `lazy`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Lucee4+ If "lazy" is set to true Lucee does not initially load all the data from the datasource.

When "true" the data is only loaded when requested, this means the data is dependent on the datasource connection. If the datasource connection has been lost for some reason and the data has not yet been requested,Lucee throws an error if you try to access the data.

The "lazy" attribute only works if the following attributes are not used:cachewithin,cacheafter and result.

### Attribute: `psq`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Lucee4+ When true preserve single quotes within the sql statement

### Attribute: `returntype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `query`
- **Description**: Lucee5+ The return type of the query result. One of the following values is accepted:
- "query": returns a query object
- "array_of_entity": returns an array of ORM entities (requires dbtype to be "hql")
- "array": returns an array of structs
- "struct": returns a struct of structs (requires columnkey to be defined).

### Attribute: `columnkey`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Lucee5+ The struct key used for each result when returntype is "struct".

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: `cfquery`

