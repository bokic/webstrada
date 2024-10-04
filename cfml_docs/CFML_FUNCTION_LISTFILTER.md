# Function Name: `ListFilter`

## Description
Used to filter an list to items for which the closure function returns true.

## Return Type
`string`

## Syntax
```cfml
listFilter(list, function(listElement, [list]) )
```

## Arguments

### Argument: `list`
- **Type**: `string`
- **Required**: Required
- **Default Value**: *None*
- **Description**: 

### Argument: `function`
- **Type**: `function`
- **Required**: Required
- **Default Value**: *None*
- **Description**: Inline closure function executed for each element in the list. Returns true if the list element should be included in the filtered list. Support for passing the original list to the closure function added in CF11 Update 5.

## Limitations and Other Info

- **Type Requirement**: Operates on list strings. Lists are 1-indexed by default and use a comma `,` as the default delimiter.
- **Related Functions**: `listGetAt`
- **Coldfusion Support**: Minimum version: `10`.
- **Lucee Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

