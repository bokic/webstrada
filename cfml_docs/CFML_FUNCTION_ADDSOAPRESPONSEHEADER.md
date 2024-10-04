# Function Name: `AddSOAPResponseHeader`

## Description
Adds a SOAP response header to a web service response. Call only from within a CFC web service function that is processing a request as a SOAP web service.

## Return Type
`boolean`

## Syntax
```cfml
addSOAPResponseHeader(namespace, name, value [, mustUnderstand])
```

## Arguments

### Argument: `namespace`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A webservice object as returned from the cfobject tag or the createobject function

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name of the SOAP header

### Argument: `value`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Value of the SOAP header

### Argument: `mustUnderstand`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The mustUnderstand attribute indicates whether processing of the header is optional or mandatory.
This basically translates to the node trying to find an appropriate handler that matches the header
and proceed with processing the message in a manner consistent with its specification. If it can't find an appropriate handler
it must return an error and stop further processing. If mustUnderstand is set to `true`
the node is not allowed to ignore it.

## Limitations and Other Info

- **Related Functions**: `getSOAPResponseHeader`, `getSOAPResponse`
- **Coldfusion Support**: Minimum version: `7`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

