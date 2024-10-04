# Function Name: `WriteDump`

## Description
Outputs the elements, variables and values of most kinds of CFML objects. Useful for debugging. You can display the contents of simple and complex variables, objects, components, user-defined functions, and other elements. Equivalent to the cfdump tag, useful in cfscript.

## Return Type
`void`

## Syntax
```cfml
writeDump(var [, output] [, format] [, abort] [, label] [, metainfo] [, top] [, show] [, hide] [, keys] [, expand] [, showUDFs])
```

## Arguments

### Argument: `var`
- **Type**: `variableName`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Variable to display. Enclose a variable name in pound
 signs.

### Argument: `output`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `browser`
- **Description**: Where to send the results of cfdump.

### Argument: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `text`
- **Description**: specify whether to save the results of a cfdump to a file in text or HTML format

### Argument: `abort`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Boolean value to immediately abort after displaying the dump.

### Argument: `label`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A string; header for the dump output.

### Argument: `metainfo`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Includes information about the query in the cfdump results.

### Argument: `top`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: The number of rows to display.

### Argument: `show`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: show column or keys.

### Argument: `hide`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: hide column or keys.

### Argument: `keys`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: For a structure, number of keys to display.

### Argument: `expand`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Yes: In Internet Explorer and Mozilla, expands views

### Argument: `showUDfs`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: show UDFs in cfdump output.

## Limitations and Other Info

- **Related Functions**: `cfdump`
- **Coldfusion Support**: Minimum version: `9`.
- **Lucee Support**: Notes: In Lucee there is a shortcut. Simply use `dump()`. Important: If using positional arguments, their order is different with Lucee (https://luceeserver.atlassian.net/browse/LDEV-2045).
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

