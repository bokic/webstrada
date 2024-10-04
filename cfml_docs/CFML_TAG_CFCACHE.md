# Tag Name: `cfcache`

## Description
Stores a copy of a page on the server and/or client computer,
 to improve page rendering performance. To do this, the tag
 creates temporary files that contain the static HTML returned
 from a CFML page.

 Use this tag if it is not necessary to get dynamic content each
 time a user accesses a page.
 You can use this tag for simple URLs and for URLs that contain
 URL parameters.

## Syntax
```cfml
<cfcache>
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: * cache: server-side and client-side caching.
 * flush: refresh cached page(s).
 * clientcache: browser-side caching only. To cache a personalized page, use this option.
 * servercache: server-side caching only. Not recommended.
 * optimal: same as "cache".

### Attribute: `directory`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Absolute path of cache directory.

### Attribute: `timespan`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The interval until the page is flushed from the cache.

### Attribute: `expireurl`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Used with action = "flush". A URL reference. CFML
 matches it against the mappings in the specified cache
 directory. Can include wildcards. For example:
 "*/view.cfm?id=*".

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A username

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A password

### Attribute: `port`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Remote port to which to connect

### Attribute: `protocol`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Protocol that is used to create URL from cache.

### Attribute: `value`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For action="set", object which needs to be stored

### Attribute: `metadata`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the struct variable

### Attribute: `stripwhitespace`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Reduces whitespace

### Attribute: `throwonerror`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A Boolean value specifying whether to throw an error if the
flush action encounters an error. Otherwise the action does not
generate an error if it fails. If this attribute is 'true' you can handle the
error in a cfcatch block, for example, if a specified id value is invalid

### Attribute: `id`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Id of the cached object

### Attribute: `key`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: key to access cache

### Attribute: `usecache`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: to use cache or not (if false it will process the content each time)

### Attribute: `dependson`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Comma separated list of all variables on which this cache would depend

### Attribute: `idletime`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Flushes the cached item if it is not accessed for the specified time span.

### Attribute: `cachedirectory`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Cache directory

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: name of return variable, valid with action="get"

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

