# Function Name: `ListMap`

## Description
Iterates over every entry of the List and calls the closure function to work on the item of the list. The returned value will be set at the same index in a new list and the new list will be returned.

## Return Type
`string`

## Syntax
```cfml
 listMap(list, function(item [,index, list]) [,delimiter, includeEmptyFields)
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: The input list.

### Argument: `function`
- **Type**: `any`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Closure or a function reference that will be called for each of the iteration. The arguments passed to the callback are

item: value
index : current index for the iteration
list : reference of the original list

### Argument: `intialValue`
- **Type**: `any`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: Initial value which will be used for the reduce operation.

### Argument: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `,`
- **Description**: The list delimiter.

### Argument: `includeEmptyFields`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Include empty values

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `ListReduce`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**: Minimum version: `4.5`.
- **Boxlang Support**: Minimum version: `1.0.0`.

