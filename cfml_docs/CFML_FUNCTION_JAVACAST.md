# Function Name: `JavaCast`

## Description
Converts the data type of a CFML variable to pass as an argument to an overloaded method of a Java object.

## Return Type
`any`

## Syntax
```cfml
javaCast(type, variable)
```

## Arguments

### Argument: `type`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of a Java primitive or a Java class name.

### Argument: `variable`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: A variable, Java object or array.

## Limitations and Other Info

- **Related Functions**: `createObject`, `cfobject`
- **Coldfusion Support**: Minimum version: `4.5`. Notes: CF7+ Added null. CF8+ added bigdecimal, byte, char, short and for casting arrays.
- **Lucee Support**: Minimum version: `4.5`.
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

