# Tag Name: `cfwebsocket`

## Description
Includes the required JavaScript files in your CFM template and creates a global JavaScript reference to the WebSocket Object on the client-side.

## Syntax
```cfml
<cfwebsocket name="" onMessage="">
```

## Attributes / Variants

### Attribute: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Attribute: `onMessage`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The JavaScript function that is called when the WebSocket receives a message from the server.

### Attribute: `onOpen`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The JavaScript function that is called when the WebSocket establishes a connection.

### Attribute: `onClose`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The JavaScript function that is called when the WebSocket closes a connection.

### Attribute: `onError`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The JavaScript function that is called if there is an error while performing an action over the WebSocket connection.

### Attribute: `usecfAuth`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: If set to true (default), users need not authenticate for WebSocket connection (provided they have already logged in to the application). This is the default value. If false, users have to specify the credentials for the WebSocket connection.

### Attribute: `subscribeTo`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Comma-separated list of channels to subscribe to. You can specify any or all channels set in your `this.wschannels` Application settings

### Attribute: `secure`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If true, the web socket communication will happen over SSL. CF11+

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

