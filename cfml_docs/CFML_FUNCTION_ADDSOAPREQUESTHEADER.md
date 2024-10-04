# Function Name: `AddSOAPRequestHeader`

## Description
Adds a SOAP header to a web service request before making the request.

## Return Type
`boolean`

## Syntax
```cfml
addSOAPRequestHeader(webservice, namespace, name, value, mustUnderstand)
```

## Arguments

### Argument: `webservice`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A webservice object as returned from the cfobject tag
 or the createobject function

### Argument: `namespace`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Namespace for the SOAP header

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of SOAP header

### Argument: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: the value for the SOAP header; this can be a CFML XML value.

### Argument: `mustUnderstand`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: The mustUnderstand attribute indicates whether processing of the header is optional or mandatory.
This basically translates to the node trying to find an appropriate handler that matches the header
and proceed with processing the message in a manner consistent with its specification. If it can't find an appropriate handler
it must return an error and stop further processing. If mustUnderstand is set to `true`
the node is not allowed to ignore it.

## Limitations and Other Info

- **Related Functions**: `getSOAPRequestHeader`, `getSOAPRequest`
- **Coldfusion Support**: Minimum version: `7`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

