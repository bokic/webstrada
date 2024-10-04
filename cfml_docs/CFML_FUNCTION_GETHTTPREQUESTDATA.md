# Function Name: `GetHttpRequestData`

## Description
Returns HTTP request headers and request body. The resulting structure contains the following keys:
	 content (the request body),
	 headers (a structure of request headers),
	 method (same as cgi.request_method),
	 protocol (same as cgi.server_protocol).

## Return Type
`struct`

## Syntax
```cfml
getHTTPRequestData()
```

## Arguments

### Argument: `includeBody`
- **Type**: `boolean`
- **Required**: Required
- **Default Value**: `false`
- **Description**: Whether return the body or not.

NOTE: This can only be done once.
If you expect the body to contain content which causes an exception in ColdFusion, set it to false as well and process it yourself.

## Limitations and Other Info

- **Related Functions**: `getpagecontext`
- **Coldfusion Support**: Minimum version: `5`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

