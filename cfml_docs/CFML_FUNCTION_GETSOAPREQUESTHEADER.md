# Function Name: `GetSOAPRequestHeader`

## Description
Obtains a SOAP request header. Call only from within a CFC web service function that is processing a request as a SOAP web service.

## Return Type
`any`

## Syntax
```cfml
getSOAPRequestHeader(namespace, name [, asXML])
```

## Arguments

### Argument: `namespace`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A String that is the namespace for the header

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A String that is the name of the header

### Argument: `asXML`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: If True, the header is returned as a CFML XML object;
 if false (default), the header is returned as a Java object.

## Limitations and Other Info

- **Related Functions**: `getSOAPRequest`, `addSOAPRequestHeader`, `getSOAPResponse`, `getSOAPResponseHeader`
- **Coldfusion Support**: Minimum version: `7`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

