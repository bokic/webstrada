# Tag Name: `cfinvoke`

## Description
Does either of the following:

 * Invokes a component method from within a CFML page or
 component.
 * Invokes a web service.
 Different attribute combinations make some attributes required
 at sometimes and not at others.

## Syntax
```cfml
<cfinvoke method="">
```

## Attributes / Variants

### Attribute: `component`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: String or component object; a reference to a component, or
 component to instantiate.

### Attribute: `method`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of a method. For a web service, the name of an
 operation.

### Attribute: `returnvariable`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of a variable for the invocation result.

### Attribute: `argumentcollection`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of a structure; associative array of arguments to pass
 to the method.

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides username specified in Administrator > Web Services

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides password specified in Administrator > Web Services

### Attribute: `webservice`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The URL of the WSDL file for the web service.

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The timeout for the web service request, in seconds

### Attribute: `proxyserver`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The proxy server required to access the webservice URL.

### Attribute: `proxyport`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The port to use on the proxy server.

### Attribute: `proxyuser`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The user ID to send to the proxy server.

### Attribute: `proxypassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The user's password on the proxy server.

### Attribute: `serviceport`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF7+ The port name for the web service. This value is
 case-sensitive and corresponds to the port element's
 name attribute under the service element. Specify this
 attribute if the web service contains multiple ports.
 Default: first port found in the WSDL.

### Attribute: `refreshwsdl`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: CF8+ * yes: reload the WSDL file and regenerate the artifacts used to consume the web service
 * no

### Attribute: `wsdl2javaargs`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF8+ A string that contains a space-delimited list of arguments to pass to the WSDL2Java tool that generates Java stubs for the web services.

### Attribute: `wsVersion`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF10+ Used to specify the version of Apache Axis (web service engine used by CF) to use. Specify 1 for Axis Version 1 or 2 for Axis 2.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

