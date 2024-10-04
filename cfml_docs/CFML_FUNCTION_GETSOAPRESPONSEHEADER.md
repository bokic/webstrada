# Function Name: `GetSOAPResponseHeader`

## Description
Returns a SOAP response header. Call this function from within code that is invoking a web service after making a web service request.

## Return Type
`any`

## Syntax
```cfml
getSOAPResponseHeader(webservice, namespace, name [, asXML])
```

## Arguments

### Argument: `webservice`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A webservice object as returned from the cfobject tag or the createObject function.

### Argument: `namespace`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A String that is the namespace for the header.

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A String that is the name of the SOAP header.

### Argument: `asXML`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: If True, the header is returned as a CFML XML object;
 if false (default), the header is returned as a Java object.

## Limitations and Other Info

- **Related Functions**: `getSOAPRequest`, `getSOAPRequestHeader`, `getSOAPResponse`
- **Coldfusion Support**: Minimum version: `7`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:

