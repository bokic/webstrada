# Function Name: `WSGetAllChannels`

## Description
Provides all the channels defined in the Application.cfc as an array.

## Return Type
`array`

## Syntax
```cfml
wsGetAllChannels (); wsGetAllChannels('channelName');
```

## Arguments

### Argument: `channelName`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: If specified, returns all sub-channels listed under the entered channel name. If left unspecified, the function returns all channels registered under the current application.

## Limitations and Other Info

- **Coldfusion Support**: Minimum version: `10`.

