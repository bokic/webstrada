# Tag Name: `cfajaxproxy`

## Description
Creates a JavaScript proxy for a ColdFusion component, for use in an AJAX client.

## Syntax
```cfml
<cfajaxproxy>
```

## Attributes / Variants

### Attribute: `cfc`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The CFC for which to create a proxy. You must specify a dot-delimited path to the CFC.
 The path can be absolute or relative to location of the CFML page.
 For example, if the myCFC CFC is in the cfcs subdirectory of the ColdFusion page, specify cfcs.myCFC.
 On UNIX based systems, the tag searches first for a file who's name or path corresponds to the specified name or path, but is in all lower case.
 If it does not find it, ColdFusion then searches for a file name or path that corresponds to the attribute value exactly, with identical character casing. (required)

### Attribute: `jsclassname`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name to use for the JavaScript proxy class. (optional)

### Attribute: `bind`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A bind expression that specifies a CFC method, JavaScript function, or URL to call.

### Attribute: `onerror`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a JavaScript function to invoke when a bind, specified by the bind attribute fails. The function must take two arguments: an error code and an error message.

### Attribute: `onsuccess`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The name of a JavaScript function to invoke when a bind, specified by the bind attribute succeeds. The function must take one argument, the bind function return value. If the bind function is a CFC function, the return value is automatically converted to a JavaScript variable before being passed to the onSuccess function.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

