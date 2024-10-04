# Function Name: `ListReduce`

## Description
Iterates over each item of the list and calls the closure to work on the item. This function will reduce the list to a single value and will return the value.

## Return Type
`any`

## Syntax
```cfml
 listReduce(list, function(result, item [,index, list]) [,initialValue, delimiter, includeEmptyFields])
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Input list

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
- **Description**: Initial value which will be used for the reduce operation. The type is any.

### Argument: `delimiter`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `comma`
- **Description**: The list delimiter.

### Argument: `includeEmptyFields`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: Include empty values.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `ArrayMap`, `ArrayReduce`
- **Coldfusion Support**: Minimum version: `11`.
- **Lucee Support**: Minimum version: `4.5`.
- **Boxlang Support**: Minimum version: `1.0.0`.

