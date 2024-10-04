# Tag Name: `cfdump`

## Description
Outputs the contents of a variable of any type for debugging purposes. The variable can be as simple as a string or as complex as a cfc component instance.

## Syntax
```cfml
<cfdump var="">
```

## Attributes / Variants

### Attribute: `var`
- **Type**: `variableName`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Variable to display. Enclose a variable name in pound signs.

### Attribute: `expand`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: Yes: Expands views

### Attribute: `label`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: A string; header for the dump output.

### Attribute: `top`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF7+ The number of rows to display. For a structure, this is the number of nested levels to display (useful for large structures).

### Attribute: `format`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `text`
- **Description**: CF8+ Specify whether to save the results of a cfdump to a file in text or HTML format.

### Attribute: `hide`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF8+ Hide column or keys.

### Attribute: `keys`
- **Type**: `numeric`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF8+ For a structure, number of keys to display.

### Attribute: `metainfo`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: CF8+ Includes information about the query in the cfdump results.

### Attribute: `output`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `browser`
- **Description**: CF8+ Where to send the results of cfdump.

### Attribute: `show`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF8+ Show column or keys.

### Attribute: `showUDfs`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `True`
- **Description**: CF8+ Show UDFs in cfdump output.

### Attribute: `abort`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: CF9+ Stops further processing of page.

## Limitations

- **Must be nested inside**: *None*
- **Must not be nested inside**: *None*

