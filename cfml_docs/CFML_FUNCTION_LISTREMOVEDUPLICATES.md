# Function Name: `ListRemoveDuplicates`

## Description
Removes duplicate values (if they exist) in a list.

## Return Type
`string`

## Syntax
```cfml
listRemoveDuplicates(list[, delimiter] [, ignoreCase])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Required. List of objects.

### Argument: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: Optional. Character(s) that separate list elements. The default value is comma.

### Argument: `ignoreCase`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `False`
- **Description**: Optional. If true, ignores the case of strings in the list. By default the value is set to false.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Coldfusion Support**: Minimum version: `10`. Notes: ColdFusion 10: Added this function
- **Railo Support**: Minimum version: `4.0`.
- **Lucee Support**: Minimum version: `4.5`.
- **Boxlang Support**: Minimum version: `1.0.0`.

