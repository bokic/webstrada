# Tag Name: `cfobject`

## Description
Creates a CFML object, of a specified type.

 The tag syntax depends on the object type. Some types use the
 type attribute; others do not.

## Syntax
```cfml
<cfobject name="">
```

## Attributes / Variants

### Attribute: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The object type. You can omit this attribute or specify component. ColdFusion automatically sets the type to component.

### Attribute: `action`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: * create: instantiates a COM object (typically, a DLL) before invoking methods or properties.
 * connect: connects to a COM object (typically, an EXE) running on server.

### Attribute: `class`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Component ProgID for the object to invoke. When using Java stubs to connect to the COM object, the class must be the ProgID of the COM object.

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: name for the instantiated component.

### Attribute: `context`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: * inproc
 * local
 * remote

In Windows, if not specified, uses Registry setting.

### Attribute: `server`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Server name, using Universal Naming Convention (UNC) or Domain Name Serve (DNS) convention, in one of these forms:

 * \\lanserver
 * lanserver
 * http://www.servername.com
 * www.servername.com
 * 127.0.0.1

### Attribute: `component`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of component to instantiate.

### Attribute: `locale`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Sets arguments for a call to init_orb. Use this attribute only for VisiBroker ORBs. It is available on C++, Version 3.2. The value must be in the form:
locale = " -ORBagentAddr 199.99.129.33 -ORBagentPort 19000"

Each type-value pair must start with a hyphen.

### Attribute: `webservice`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: One of the following:

 * The absolute URL of the web service.
 * The name (string) assigned in the ColdFusion Administrator to the web service.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The password to use to access the web service. If the webservice attribute specifies a web service name configured in the ColdFusion Administrator, overrides any password specified in the Administrator entry.

### Attribute: `secure`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Whether to secure communications with the .NET-side agent. If true, ColdFusion uses SSL to communicate with .NET.

### Attribute: `protocol`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `tcp`
- **Description**: Protocol to use to use for communication between ColdFusion and .NET. Must be one of the following values:
 * http: Use HTTP/SOAP communication protocol. This option is slower than tcp, but might be required for access through a firewall.
 * tcp: Use binary TCP/IP protocol. This method is more efficient than HTTP.

### Attribute: `proxyserver`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The proxy server required to access the web service URL.

### Attribute: `refreshwsdl`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: * yes: reloads the WSDL file and regenerates the artifacts used to consume the web service
 * no

### Attribute: `wsportname`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The port name for the web service. This value is case-sensitive and corresponds to the port element's name attribute under the service element.

### Attribute: `wsdl2javaargs`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A string that contains a space-delimited list of arguments to pass to the WSDL2Java tool that generates Java stubs for the web services.

### Attribute: `proxyport`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The port to use on the proxy server.

### Attribute: `port`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Port number at which the .NET-side agent is listening.

### Attribute: `proxypassword`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The user's password on the proxy server.

### Attribute: `assembly`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For local .NET assemblies, the absolute path or paths to the assembly or assemblies (EXE or DLL files) from which to access the .NET class and its supporting classes.
For remote .NET assemblies, you must specify the absolute path or paths of the local proxy JAR file or files that represent the assemblies.

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The user name to use to access the web service. If the webservice attribute specifies a web service configured name in the ColdFusion Administrator, overrides any user name specified in the Administrator entry.

### Attribute: `proxyuser`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The user ID to send to the proxy server.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

