# Function Name: `CreateObject`

## Description
The createObject function takes different arguments depending on the value of the type argument:

 createObject('component', cfcName)
 createObject('java', class)
 createObject('java', class, bundleName, bundleVersion) (Lucee only)
 createObject('webservice', urltowsdl, [, portname])
 createObject('.NET', class, assembly [, server, port, protocol, secure])
 createObject('com', class, context, serverName)

## Return Type
`any`

## Syntax
```cfml
createObject(type, className)
```

## Arguments

### Argument: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The type of object

### Argument: `className`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `context`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `locale`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `servername`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `component_name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `urltowsdl`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: WSDL file URL; location of web service

### Argument: `portname`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The port name for the web service. This value is case-sensitive
 and corresponds to the port element's name attribute under the
 service element.
 Specify this parameter if the web service contains multiple ports.
 If no port name is specified, ColdFusion uses the first port found
 in the WSDL.

### Argument: `bundleName`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Bundle where the object has to be located

### Argument: `bundleVersion`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specific version to

## Limitations and Other Info

- **Related Functions**: `cfobject`, `cfinvoke`
- **Coldfusion Support**: Minimum version: `4.5`. Notes: CORBA support DEPRECATED in CF11+
- **Lucee Support**: Notes: OSGi bundle and version ADDED in Lucee5+
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

