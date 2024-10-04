# Function Name: `WSPublish`

## Description
 Sends messages to a specific channel based on the filter criteria (which is a struct).

## Return Type
`void`

## Syntax
```cfml
wsPublish(String channel, Object message); wsPublish(channel,message [,filterCriteria]);
```

## Arguments

### Argument: `channel`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Specific channel to which the server publishes its response.

### Argument: `message`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Response sent by the server to all clients subscribed to a specific channel.

### Argument: `filterCriteria`
- **Type**: `struct`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Conditions to filter eligible clients that need to be notified for a given channel.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `10`.

