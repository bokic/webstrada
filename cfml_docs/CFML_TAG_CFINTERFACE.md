# Tag Name: `cfinterface`

## Description
Defines an interface that consists of a set of signatures for functions.
 The interface does not include the full function definitions;
 instead, you implement the functions in a CFC.
 The interfaces that you define by using this tag can make up
 the structure of a reusable application framework.

## Syntax
```cfml
<cfinterface>
```

## Attributes / Variants

### Attribute: `displayName`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A value to be displayed when using introspection to
 show a descriptive name for the interface. (optional)

### Attribute: `extends`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A comma delimited list of one or more interfaces that this interface extends.
 Any CFC that implements an interface must also implement all the functions
 in the interfaces specified by this property.
 If an interface extends another interface, and the child interface specifies
 a function with the same name as one in the parent interface, both functions
 must have the same attributes; otherwise ColdFusion generates an error. (optional)

### Attribute: `hint`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Text to be displayed when using introspection to show information about the interface.
 The hint attribute value follows the syntax line in the function description. (optional)

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

