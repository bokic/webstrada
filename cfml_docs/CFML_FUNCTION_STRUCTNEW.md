# Function Name: `StructNew`

## Description
Creates a new, empty structure. The shorthand syntax for an empty unordered struct is `{}`. You can also use the syntax `{"key":"value"}` to initialize it with values. The shorthand syntax for an ordered struct is `[:]` or `[=]`. The shorthand for a case-sensitive struct is `${}`.The shorthand for a ordered and casesensitive struct is `$[=]`.
NOTE: To preserve the case of the struct key, place quotes around it, otherwise it will be converted to uppercase.

## Return Type
`struct`

## Syntax
```cfml
structNew([type[[,sortType][,sortOrder][,localeSensitive]|[,callback]]])
```

## Arguments

### Argument: `type`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF2016+ Lucee4.5+ If set to `ordered` the order in which elements are added to the structure will be maintained. In Lucee `linked` can be used in place of `ordered`.
CF2021+ If set to `casesensitive` the keys will remain case-sensitive. Additionally, `ordered-casesensitive` can be used to create an ordered case-sensitive struct.

### Argument: `sortType`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF2016u3+ Sort types are text or numeric.

### Argument: `sortOrder`
- **Type**: `string`
- **Required**: Optional
- **Default Value**: `asc`
- **Description**: CF2016u3+ The order of the sort (ascending or descending).

### Argument: `localeSensitive`
- **Type**: `boolean`
- **Required**: Optional
- **Default Value**: `false`
- **Description**: CF2016u3+ Specify if you wish to do a locale sensitive sorting.

### Argument: `callback`
- **Type**: `function`
- **Required**: Optional
- **Default Value**: *None*
- **Description**: CF2016u3+ A comparator function used to sort new data entered into the structure. Returns 1, 0 or -1.

## Limitations and Other Info

- **Type Requirement**: The first argument must be a valid structure/associative array.
- **Related Functions**: `struct-functions`
- **Coldfusion Support**: Notes: CF2016 added ordered structs. CF2016u3 added sorted structs. CF2018 added named parameters. CF2021 added 2 new types: ordered-casesensitive & casesensitive.
- **Lucee Support**:
- **Railo Support**:
- **Openbd Support**:
- **Boxlang Support**: Minimum version: `1.0.0`.

