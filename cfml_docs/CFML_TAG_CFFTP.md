# Tag Name: `cfftp`

## Description
Lets users implement File Transfer Protocol (FTP) operations.

## Syntax
```cfml
<cfftp action="open">
```

## Attributes / Variants

### Attribute: `action`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: FTP operation to perform.
 open: create an FTP connection
 close: terminate an FTP connection

### Attribute: `username`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides username specified in ODBC setup.

### Attribute: `password`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Overrides password specified in ODBC setup.

### Attribute: `server`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: FTP server to which to connect; for example,
 ftp.myserver.com

### Attribute: `timeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `30`
- **Description**: Value in seconds for the timeout of all operations,
 including individual data request operations.

### Attribute: `port`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `21`
- **Description**: Remote port to which to connect

### Attribute: `connection`
- **Type**: `variableName`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the FTP connection. If you specify the username,
 password, and server attributes, and if no connection
 exists for them, CFML creates one. Calls to cfftp
 with the same connection name reuse the connection.

### Attribute: `proxyserver`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The proxy server required to access the URL.

### Attribute: `retrycount`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `1`
- **Description**: Number of retries until failure is reported.

### Attribute: `stoponerror`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: halts processing, displays an appropriate error.
 No: populates the error variables

### Attribute: `passive`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Yes: enable passive mode

### Attribute: `transfermode`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `auto`
- **Description**: ASCII FTP transfer mode
 Binary FTP transfer mode
 Auto FTP transfer mode

### Attribute: `failifexists`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Yes: if a local file with same name exists, getFile fails

### Attribute: `directory`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Directory on which to perform an operation

### Attribute: `localfile`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the file on the local file system

### Attribute: `remotefile`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Name of the file on the FTP server file system.

### Attribute: `item`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Object of these actions: file or directory.

### Attribute: `existing`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Current name of the file or directory on the remote server.

### Attribute: `new`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: New name of file or directory on the remote server

### Attribute: `name`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Query name of directory listing.

### Attribute: `result`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Specifies a name for the structure in which cfftp
 stores the returnValue variable. If set, this value
 replaces cfftp as the prefix to use when accessing
 returnVariable.

### Attribute: `attributes`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Attributes of the current element: normal or Directory.

### Attribute: `passphrase`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF8+ Used when `key` is specified. Because private keys are stored in an encrypted form on the client host, the user must supply a passphrase to enable generating the signature.

### Attribute: `buffersize`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Buffer size in bytes.

### Attribute: `secure`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: CF8+ `yes`: enables secure FTP

### Attribute: `asciiextensionlist`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Delimited list of file extensions that force ASCII
 transfer mode, if transferMode = "auto".

### Attribute: `key`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF8+ Public-key-based authentication. Refers to the absolute path to the private key of the user. 
Possession of a private key provides authentication by sending a signature created with a private key. 
The server must ensure that the key is a valid authentication for the user and that the signature is valid. 
Both must be valid to accept the authentication.

### Attribute: `actionparam`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Used only when action is quote, site, or acct. Specifies the command when action is quote or site; specifies account information when action is acct.

### Attribute: `fingerprint`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF8+ Fingerprint of the host key in the form ssh-dss.ssh-rsa, which is a 16-byte unique identifier for the server attribute that you specify.

### Attribute: `systemtype`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF11.0.3+ Specifies how to parse file list response, specify `WINDOWS` or `UNIX` or a class which implements `org.apache.commons.net.ftp.FTPFileEntryParser`

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

