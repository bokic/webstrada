# Function Name: `ToScript`

## Description
 Creates a JavaScript or ActionScript expression that
 assigns the value of a ColdFusion variable to a JavaScript
 or ActionScript variable. This function can convert
 ColdFusion strings, numbers, arrays, structures, and
 queries to JavaScript or ActionScript syntax that defines
 equivalent variables and values.

## Return Type
`string`

## Syntax
```cfml
toScript(cfvar, javascriptvar [, outputformat] [, asformat])
```

## Arguments

### Argument: `cfvar`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A ColdFusion variable. This can contain one of the following:
 String, Number, Array, Structure or Query.

### Argument: `javascriptvar`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string that specifies the name of the JavaScript variable
 that the toScript function creates.

### Argument: `outputformat`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: A Boolean value that determines whether to create
 WDDX (JavaScript) or ActionScript style output for
 structures and queries.
 Default: true

### Argument: `asformat`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: A Boolean value that specifies whether to use
 ActionScript shortcuts in the script.
 Default: false

## Limitations and Other Info

- **Related Functions**: `cfwddx`
- **Coldfusion Support**:
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

