# Tag Name: `cfclientsettings`

## Description
Part of the new CF11 mobile development features. This tag is similar to cfprocessingdirective and acts as a compiler directive to include plugins for various features (device detection and device API). You can use this tag to load all the required device detection plugins.

## Syntax
```cfml
 <cfclientsettings enableDeviceAPI = "true|false" detectDevice = "true|false" deviceTimeout = Number > 
```

## Attributes / Variants

### Attribute: `enableDeviceAPI`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Enable/disable the device API

### Attribute: `detectDevice`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Enable/disable the device detection plugin.

### Attribute: `deviceTimeout`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: `10`
- **Description**: The timeout for loading the plugins (in seconds).

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

