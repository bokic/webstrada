# Function Name: `Invoke`

## Description
Invokes an object method and returns the result of the invoked method.

## Return Type
`any`

## Syntax
```cfml
invoke(instance, methodName [, arguments])
```

## Arguments

### Argument: `instance`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Name or instance of a CFC or an instance of a Java, .NET, COM or CORBA object to instantiate. For a CFC, it can be an empty string when invoking a method within the same ColdFusion page or component.

### Argument: `methodname`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of the method (or operation for webservice) to invoke.

### Argument: `arguments`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: An array of positional arguments or a struct of named arguments to pass into the method.

## Limitations and Other Info

- **Related Functions**: `cfinvoke`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

