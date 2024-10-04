# Function Name: `IsDefined`

## Description
Evaluates a string value to determine whether the variable named in it exists.

## Return Type
`boolean`

## Syntax
```cfml
isDefined(variable_name)
```

## Arguments

### Argument: `variable_name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A string expression containing the name of a variable to check the existence of. Only simple dotted notation is supported (for example `myVar`, `arguments.myArg`, `myStruct.key`). Square bracket notation to reference array elements or struct keys is not supported.

## Limitations and Other Info

- **Related Functions**: `structKeyExists`, `isEmpty`, `ArrayIsDefined`
- **Coldfusion Support**: Minimum version: `3`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

