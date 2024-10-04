# Function Name: `DeleteClientVariable`

## Description
 Deletes a client variable. Returns `true` if variable was successfully deleted; `false` if it was not deleted.
NOTE: To test for the existence of a variable, use `IsDefined` or `structKeyExists`.)

## Return Type
`boolean`

## Syntax
```cfml
deleteClientVariable(name)
```

## Arguments

### Argument: `name`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The name of a client variable to delete, surrounded by double-quotation marks.

## Limitations and Other Info

- **Related Functions**: `getClientVariablesList`
- **Coldfusion Support**: Notes: ColdFusion MX: Changed behavior: if the variable is not present, this function now returns False. (In earlier releases, it threw an error.)
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

